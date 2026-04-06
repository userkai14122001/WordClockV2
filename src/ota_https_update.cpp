#include "ota_https_update.h"
#include "DebugManager.h"
#include "SystemControl.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_https_ota.h>
#include <ArduinoJson.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.0.0-dev"
#endif

namespace {
    static const char* kManifestUrl =
        "https://raw.githubusercontent.com/userkai14122001/WordClockV2/main/ota_manifest.json";

    struct RemoteOtaInfo {
        String version;
        String firmwareUrl;
    };

    static bool parseVersionNumbers(String input, int out[3]) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;

        input.trim();
        if (input.startsWith("v") || input.startsWith("V")) {
            input.remove(0, 1);
        }

        int start = 0;
        for (int i = 0; i < 3; i++) {
            int dot = input.indexOf('.', start);
            String part = (dot >= 0) ? input.substring(start, dot) : input.substring(start);
            part.trim();

            if (part.isEmpty()) {
                out[i] = 0;
            } else {
                for (size_t k = 0; k < part.length(); k++) {
                    if (!isDigit(part[k])) {
                        return false;
                    }
                }
                out[i] = part.toInt();
            }

            if (dot < 0) {
                break;
            }
            start = dot + 1;
        }
        return true;
    }

    static int compareSemver(const String& current, const String& remote) {
        int a[3] = {0, 0, 0};
        int b[3] = {0, 0, 0};
        if (!parseVersionNumbers(current, a) || !parseVersionNumbers(remote, b)) {
            return 0;
        }

        for (int i = 0; i < 3; i++) {
            if (a[i] < b[i]) return -1;
            if (a[i] > b[i]) return 1;
        }
        return 0;
    }

    static bool fetchRemoteOtaInfo(RemoteOtaInfo& info) {
        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;
        if (!http.begin(client, kManifestUrl)) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Manifest-Verbindung fehlgeschlagen");
            return false;
        }

        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setTimeout(10000);
        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            DebugManager::printf(DebugCategory::OTA, "[OTA] Manifest HTTP Fehler: %d\n", code);
            http.end();
            return false;
        }

        String payload = http.getString();
        http.end();

        DynamicJsonDocument doc(768);
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            DebugManager::printf(DebugCategory::OTA, "[OTA] Manifest JSON Fehler: %s\n", err.c_str());
            return false;
        }

        const char* version = doc["version"] | "";
        const char* firmwareUrl = doc["firmware_url"] | "";

        if (strlen(version) == 0 || strlen(firmwareUrl) == 0) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Manifest unvollstaendig (version/firmware_url)");
            return false;
        }

        info.version = version;
        info.firmwareUrl = firmwareUrl;
        return true;
    }
}

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

const char* getFirmwareVersion() {
    return FIRMWARE_VERSION;
}

void performHttpsOtaUpdate(const char* firmwareUrl) {
    if (firmwareUrl == nullptr || strlen(firmwareUrl) == 0) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Keine Firmware-URL angegeben");
        return;
    }

    DebugManager::println(DebugCategory::OTA, "[OTA] Starte HTTPS OTA Update...");
    DebugManager::printf(DebugCategory::OTA, "[OTA] URL: %s\n", firmwareUrl);

    esp_http_client_config_t http_config = {};
    http_config.url = firmwareUrl;
    http_config.timeout_ms = 10000;
    http_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_config.skip_cert_common_name_check = true;
    http_config.use_global_ca_store = false;
    http_config.cert_pem = NULL;

    esp_err_t ret = esp_https_ota(&http_config);

    if (ret == ESP_OK) {
        DebugManager::println(DebugCategory::OTA, "[OTA] OTA erfolgreich! Neustart...");
        rebootDevice("OTA success", 1000);
    } else {
        DebugManager::printf(DebugCategory::OTA, "[OTA] OTA fehlgeschlagen! Fehlercode: %d\n", ret);
    }
}

bool checkForUpdateAndInstall(bool forceLog) {
    if (!WiFi.isConnected()) {
        if (forceLog) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Kein WLAN - Updatepruefung uebersprungen");
        }
        return false;
    }

    RemoteOtaInfo remote;
    if (!fetchRemoteOtaInfo(remote)) {
        if (forceLog) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Manifest konnte nicht geladen werden");
        }
        return false;
    }

    const String current = getFirmwareVersion();
    const int cmp = compareSemver(current, remote.version);

    DebugManager::printf(DebugCategory::OTA,
                         "[OTA] Version lokal=%s remote=%s\n",
                         current.c_str(),
                         remote.version.c_str());

    if (cmp < 0) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Neuere Version gefunden - update wird gestartet");
        performHttpsOtaUpdate(remote.firmwareUrl.c_str());
        return true;
    }

    if (forceLog) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Keine neuere Version gefunden");
    }
    return false;
}
