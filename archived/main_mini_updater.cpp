#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <ArduinoJson.h>

#ifndef OTA_CHANNEL
#define OTA_CHANNEL "stable"
#endif

static const char* kManifestUrl =
    "https://raw.githubusercontent.com/userkai14122001/WordClockV2/main/ota_manifest.json";

static constexpr unsigned long kWiFiConnectTimeoutMs = 30000UL;
static constexpr unsigned long kRetryIntervalMs = 15000UL;

struct RemoteFirmwareInfo {
    String version;
    String firmwareUrl;
};

Preferences gPrefs;
unsigned long gLastAttemptMs = 0;

static bool loadStoredWifiCredentials(String& ssid, String& pass) {
    if (!gPrefs.begin("wifi", true)) {
        return false;
    }
    ssid = gPrefs.getString("ssid", "");
    pass = gPrefs.getString("wifi_pass", "");
    gPrefs.end();

    ssid.trim();
    pass.trim();
    return !ssid.isEmpty();
}

static bool connectWifiWithStoredCredentials() {
    String ssid;
    String pass;
    if (!loadStoredWifiCredentials(ssid, pass)) {
        Serial.println("[MINI] Keine gespeicherten WLAN-Credentials gefunden.");
        return false;
    }

    Serial.printf("[MINI] Verbinde WLAN: %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    const unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < kWiFiConnectTimeoutMs) {
        delay(200);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MINI] WLAN-Verbindung fehlgeschlagen.");
        return false;
    }

    Serial.printf("[MINI] WLAN verbunden, IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

static bool fetchLatestFirmwareInfo(RemoteFirmwareInfo& out) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, kManifestUrl)) {
        Serial.println("[MINI] Manifest-Verbindung fehlgeschlagen.");
        return false;
    }

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[MINI] Manifest HTTP Fehler: %d\n", code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    if (payload.length() >= 3 &&
        (uint8_t)payload[0] == 0xEF &&
        (uint8_t)payload[1] == 0xBB &&
        (uint8_t)payload[2] == 0xBF) {
        payload.remove(0, 3);
    }

    DynamicJsonDocument doc(1024);
    const DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[MINI] Manifest JSON Fehler: %s\n", err.c_str());
        return false;
    }

    JsonVariant selected = doc.as<JsonVariant>();
    JsonVariant channels = doc["channels"];
    if (!channels.isNull() && channels.is<JsonObject>()) {
        JsonVariant channelNode = channels[OTA_CHANNEL];
        if (channelNode.isNull() || !channelNode.is<JsonObject>()) {
            Serial.printf("[MINI] Kanal '%s' nicht im Manifest gefunden.\n", OTA_CHANNEL);
            return false;
        }
        selected = channelNode;
    }

    out.version = String((const char*)(selected["version"] | ""));
    out.firmwareUrl = String((const char*)(selected["firmware_url"] | ""));

    out.version.trim();
    out.firmwareUrl.trim();

    if (out.version.isEmpty() || out.firmwareUrl.isEmpty()) {
        Serial.println("[MINI] Manifest unvollstaendig (version/firmware_url).\n");
        return false;
    }

    Serial.printf("[MINI] Zielversion: %s\n", out.version.c_str());
    return true;
}

static bool otaInstallFromUrl(const String& url) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, url)) {
        Serial.println("[MINI] OTA URL ungueltig.");
        return false;
    }

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(20000);
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[MINI] OTA HTTP Fehler: %d\n", code);
        http.end();
        return false;
    }

    const int contentLength = http.getSize();
    if (contentLength <= 0) {
        Serial.println("[MINI] OTA Content-Length ungueltig.");
        http.end();
        return false;
    }

    if (!Update.begin((size_t)contentLength, U_FLASH)) {
        Serial.printf("[MINI] Update.begin fehlgeschlagen: %s\n", Update.errorString());
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buffer[1024];

    while (http.connected() && written < (size_t)contentLength) {
        size_t avail = stream->available();
        if (avail == 0) {
            delay(1);
            continue;
        }

        if (avail > sizeof(buffer)) {
            avail = sizeof(buffer);
        }
        if ((size_t)contentLength - written < avail) {
            avail = (size_t)contentLength - written;
        }

        int readBytes = stream->readBytes(buffer, avail);
        if (readBytes <= 0) {
            http.end();
            Update.abort();
            Serial.println("[MINI] Stream-Lesefehler.");
            return false;
        }

        size_t chunkWritten = Update.write(buffer, (size_t)readBytes);
        if (chunkWritten != (size_t)readBytes) {
            http.end();
            Update.abort();
            Serial.println("[MINI] Flash-Schreibfehler.");
            return false;
        }

        written += (size_t)readBytes;
    }

    http.end();

    if (!Update.end()) {
        Serial.printf("[MINI] Update.end fehlgeschlagen: %s\n", Update.errorString());
        return false;
    }

    if (!Update.isFinished()) {
        Serial.println("[MINI] Update nicht abgeschlossen.");
        return false;
    }

    Serial.println("[MINI] OTA erfolgreich. Neustart...");
    delay(500);
    ESP.restart();
    return true;
}

static void tryBootstrapUpdate() {
    if (!connectWifiWithStoredCredentials()) {
        return;
    }

    RemoteFirmwareInfo info;
    if (!fetchLatestFirmwareInfo(info)) {
        return;
    }

    Serial.printf("[MINI] Starte OTA von %s\n", info.firmwareUrl.c_str());
    if (!otaInstallFromUrl(info.firmwareUrl)) {
        Serial.println("[MINI] OTA fehlgeschlagen, versuche spaeter erneut.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[MINI] WordClock Mini Updater startet...");
    gLastAttemptMs = 0;
}

void loop() {
    const unsigned long nowMs = millis();
    if (gLastAttemptMs == 0 || (nowMs - gLastAttemptMs) >= kRetryIntervalMs) {
        gLastAttemptMs = nowMs;
        tryBootstrapUpdate();
    }
    delay(100);
}
