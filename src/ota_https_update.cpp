#include "ota_https_update.h"
#include "DebugManager.h"
#include "SystemControl.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <esp_https_ota.h>
#include <esp_crt_bundle.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.0.0-dev"
#endif

#ifndef OTA_CHANNEL
#define OTA_CHANNEL "stable"
#endif

namespace {
    static const char* kManifestUrl =
        "https://raw.githubusercontent.com/userkai14122001/WordClockV2/main/ota_manifest.json";

    struct RemoteOtaInfo {
        String version;
        String firmwareUrl;
        String sha256;
        String channel;
    };

    static String toLowerCopy(const String& value) {
        String out = value;
        out.toLowerCase();
        return out;
    }

    static String normalizeSha256(String value) {
        value.trim();
        value.toLowerCase();
        value.replace(" ", "");
        return value;
    }

    static String bytesToHexLower(const uint8_t* bytes, size_t len) {
        static const char* kHex = "0123456789abcdef";
        String out;
        out.reserve(len * 2);
        for (size_t i = 0; i < len; i++) {
            out += kHex[(bytes[i] >> 4) & 0x0F];
            out += kHex[bytes[i] & 0x0F];
        }
        return out;
    }

    static String withCacheBuster(const String& url) {
        String out = url;
        out += (url.indexOf('?') >= 0) ? "&" : "?";
        out += "cb=";
        out += String((unsigned long)millis());
        out += "-";
        out += String((unsigned long)esp_random(), HEX);
        return out;
    }

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

        const String manifestUrl = withCacheBuster(String(kManifestUrl));

        HTTPClient http;
        if (!http.begin(client, manifestUrl)) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Manifest-Verbindung fehlgeschlagen");
            return false;
        }

        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setTimeout(10000);
        http.addHeader("Cache-Control", "no-cache, no-store, max-age=0");
        http.addHeader("Pragma", "no-cache");
        http.addHeader("Expires", "0");
        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            DebugManager::printf(DebugCategory::OTA, "[OTA] Manifest HTTP Fehler: %d\n", code);
            http.end();
            return false;
        }

        String payload = http.getString();
        http.end();

        // Be tolerant if the upstream still serves UTF-8 BOM.
        if (payload.length() >= 3 &&
            (uint8_t)payload[0] == 0xEF &&
            (uint8_t)payload[1] == 0xBB &&
            (uint8_t)payload[2] == 0xBF) {
            payload.remove(0, 3);
        }

        DynamicJsonDocument doc(768);
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            DebugManager::printf(DebugCategory::OTA, "[OTA] Manifest JSON Fehler: %s\n", err.c_str());
            return false;
        }

        JsonVariant selected = doc.as<JsonVariant>();

        // Optional release channels in manifest:
        // {
        //   "channels": {
        //     "stable": {"version":"...","firmware_url":"...","sha256":"..."},
        //     "dev":    {"version":"...","firmware_url":"...","sha256":"..."}
        //   }
        // }
        JsonVariant channels = doc["channels"];
        if (!channels.isNull() && channels.is<JsonObject>()) {
            JsonVariant channelNode = channels[OTA_CHANNEL];
            if (!channelNode.isNull() && channelNode.is<JsonObject>()) {
                selected = channelNode;
                info.channel = OTA_CHANNEL;
            } else {
                DebugManager::printf(DebugCategory::OTA,
                                     "[OTA] Manifest channels vorhanden, Kanal '%s' fehlt\n",
                                     OTA_CHANNEL);
                return false;
            }
        } else {
            info.channel = OTA_CHANNEL;
        }

        const char* version = selected["version"] | "";
        const char* firmwareUrl = selected["firmware_url"] | "";
        const char* sha256 = selected["sha256"] | "";

        if (strlen(version) == 0 || strlen(firmwareUrl) == 0) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Manifest unvollstaendig (version/firmware_url)");
            return false;
        }

        info.version = version;
        info.firmwareUrl = firmwareUrl;
        info.sha256 = normalizeSha256(String(sha256));
        if (!info.sha256.isEmpty() && info.sha256.length() != 64) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Manifest SHA256 ungueltig (nicht 64 hex chars)");
            return false;
        }
        return true;
    }

    static bool performHttpsOtaViaHttpClient(const char* firmwareUrl, const char* expectedSha256) {
        WiFiClientSecure client;
        client.setInsecure();

        const String cacheBustedFirmwareUrl = withCacheBuster(String(firmwareUrl));

        HTTPClient http;
        if (!http.begin(client, cacheBustedFirmwareUrl)) {
            DebugManager::println(DebugCategory::OTA, "[OTA] HTTPClient begin fehlgeschlagen");
            return false;
        }

        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setTimeout(20000);
        http.addHeader("Cache-Control", "no-cache, no-store, max-age=0");
        http.addHeader("Pragma", "no-cache");
        http.addHeader("Expires", "0");
        const int code = http.GET();
        if (code != HTTP_CODE_OK) {
            DebugManager::printf(DebugCategory::OTA,
                                 "[OTA] HTTPClient download fehlgeschlagen: %d\n",
                                 code);
            http.end();
            return false;
        }

        const int contentLength = http.getSize();
        if (contentLength <= 0) {
            DebugManager::println(DebugCategory::OTA,
                                  "[OTA] HTTPClient ungueltige Content-Length");
            http.end();
            return false;
        }

        if (!Update.begin((size_t)contentLength, U_FLASH)) {
            DebugManager::printf(DebugCategory::OTA,
                                 "[OTA] Update.begin fehlgeschlagen: %s\n",
                                 Update.errorString());
            http.end();
            return false;
        }

        WiFiClient* stream = http.getStreamPtr();
        size_t written = 0;
        bool streamError = false;

        mbedtls_sha256_context shaCtx;
        const bool verifyHash = (expectedSha256 != nullptr && strlen(expectedSha256) > 0);
        if (verifyHash) {
            mbedtls_sha256_init(&shaCtx);
            mbedtls_sha256_starts_ret(&shaCtx, 0);
        }

        uint8_t buffer[1024];
        while (http.connected() && (written < (size_t)contentLength)) {
            size_t avail = stream->available();
            if (avail == 0) {
                delay(1);
                continue;
            }

            size_t toRead = avail;
            if (toRead > sizeof(buffer)) toRead = sizeof(buffer);
            if ((size_t)contentLength - written < toRead) {
                toRead = (size_t)contentLength - written;
            }

            const int readBytes = stream->readBytes(buffer, toRead);
            if (readBytes <= 0) {
                streamError = true;
                break;
            }

            const size_t chunkWritten = Update.write(buffer, (size_t)readBytes);
            if (chunkWritten != (size_t)readBytes) {
                streamError = true;
                break;
            }

            if (verifyHash) {
                mbedtls_sha256_update_ret(&shaCtx, buffer, (size_t)readBytes);
            }
            written += (size_t)readBytes;
        }

        String actualSha;
        if (verifyHash && !streamError) {
            uint8_t digest[32];
            mbedtls_sha256_finish_ret(&shaCtx, digest);
            actualSha = bytesToHexLower(digest, sizeof(digest));
            mbedtls_sha256_free(&shaCtx);
        } else if (verifyHash) {
            mbedtls_sha256_free(&shaCtx);
        }

        const bool complete = Update.end();
        http.end();

        if (streamError) {
            DebugManager::println(DebugCategory::OTA,
                                  "[OTA] Stream-Fehler beim Download/Write");
            return false;
        }

        if (!complete) {
            DebugManager::printf(DebugCategory::OTA,
                                 "[OTA] Update.end fehlgeschlagen: %s\n",
                                 Update.errorString());
            return false;
        }

        if (written != (size_t)contentLength) {
            DebugManager::printf(DebugCategory::OTA,
                                 "[OTA] Bytes unvollstaendig: %u/%u\n",
                                 (unsigned)written,
                                 (unsigned)contentLength);
            return false;
        }

        if (!Update.isFinished()) {
            DebugManager::println(DebugCategory::OTA, "[OTA] Update nicht abgeschlossen");
            return false;
        }

        if (verifyHash) {
            const String expected = normalizeSha256(String(expectedSha256));
            if (expected != actualSha) {
                DebugManager::printf(DebugCategory::OTA,
                                     "[OTA] SHA256 mismatch expected=%s actual=%s\n",
                                     expected.c_str(), actualSha.c_str());
                return false;
            }
            DebugManager::println(DebugCategory::OTA, "[OTA] SHA256 verifiziert");
        }

        return true;
    }
}

const char* getFirmwareVersion() {
    return FIRMWARE_VERSION;
}

const char* getOtaChannel() {
    return OTA_CHANNEL;
}

void performHttpsOtaUpdate(const char* firmwareUrl, const char* expectedSha256) {
    if (firmwareUrl == nullptr || strlen(firmwareUrl) == 0) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Keine Firmware-URL angegeben");
        return;
    }

    DebugManager::println(DebugCategory::OTA, "[OTA] Starte HTTPS OTA Update...");
    const String cacheBustedFirmwareUrl = withCacheBuster(String(firmwareUrl));
    DebugManager::printf(DebugCategory::OTA, "[OTA] URL: %s\n", cacheBustedFirmwareUrl.c_str());

    if (expectedSha256 != nullptr && strlen(expectedSha256) > 0) {
        DebugManager::println(DebugCategory::OTA,
                              "[OTA] Manifest SHA256 vorhanden - verifizierenden Download-Pfad nutzen");
        if (performHttpsOtaViaHttpClient(firmwareUrl, expectedSha256)) {
            DebugManager::println(DebugCategory::OTA, "[OTA] OTA erfolgreich! Neustart...");
            rebootDevice("OTA success", 1000);
        } else {
            DebugManager::println(DebugCategory::OTA, "[OTA] OTA fehlgeschlagen (SHA256 Verifikation)");
        }
        return;
    }

    esp_http_client_config_t http_config = {};
    http_config.url = cacheBustedFirmwareUrl.c_str();
    http_config.timeout_ms = 15000;
    http_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    // GitHub OTA requires TLS server verification. Use ESP-IDF root cert bundle.
    http_config.crt_bundle_attach = arduino_esp_crt_bundle_attach;
    http_config.skip_cert_common_name_check = false;

    esp_err_t ret = esp_https_ota(&http_config);
    if (ret != ESP_OK) {
        DebugManager::printf(DebugCategory::OTA,
                             "[OTA] HTTPS OTA strict TLS fehlgeschlagen (%d), retry mit relaxter CN-Pruefung...\n",
                             ret);
        http_config.skip_cert_common_name_check = true;
        ret = esp_https_ota(&http_config);
    }

    if (ret != ESP_OK) {
        DebugManager::println(DebugCategory::OTA,
                              "[OTA] esp_https_ota fehlgeschlagen, fallback via HTTPClient/Update...");
        if (performHttpsOtaViaHttpClient(firmwareUrl, expectedSha256)) {
            DebugManager::println(DebugCategory::OTA,
                                  "[OTA] Fallback OTA via HTTPClient erfolgreich");
            ret = ESP_OK;
        }
    }

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
                         "[OTA] Kanal=%s Version lokal=%s remote=%s\n",
                         remote.channel.c_str(),
                         current.c_str(),
                         remote.version.c_str());

    if (cmp < 0) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Neuere Version gefunden - update wird gestartet");
        performHttpsOtaUpdate(remote.firmwareUrl.c_str(), remote.sha256.c_str());
        return true;
    }

    if (forceLog) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Keine neuere Version gefunden");
    }
    return false;
}
