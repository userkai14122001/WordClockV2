#include "WiFiManager.h"
#include "web_pages.h"
#include "StateManager.h"
#include "EffectManager.h"
#include "MQTTManager.h"
#include "RTCManager.h"
#include "MemoryManager.h"
#include "DebugManager.h"
#include "SystemControl.h"
#include "ota_https_update.h"
#include "ControlConfig.h"
#include "matrix.h"
#include "effects.h"
#include "WordClockLayout.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
#include <time.h>
#include <esp_timer.h>

#ifndef WC_API_KEY
#define WC_API_KEY ""
#endif

static const byte DNS_PORT = 53;
static constexpr uint64_t HTTP_WRITE_WINDOW_US = 60ULL * 60ULL * 1000000ULL; // 1 hour since boot
static const char* kWiFiConfigBackupPath = "/wifi_config.json";

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

static String normalizeOtaProfile(String profile) {
    profile.trim();
    profile.toLowerCase();
    if (profile == "dev" || profile == "norm" || profile == "long") {
        return profile;
    }
    return "long";
}

static bool isHexColor6(const String& value) {
    if (value.length() != 6) return false;
    for (size_t i = 0; i < value.length(); i++) {
        const char c = value[i];
        const bool isHex = (c >= '0' && c <= '9') ||
                           (c >= 'a' && c <= 'f') ||
                           (c >= 'A' && c <= 'F');
        if (!isHex) return false;
    }
    return true;
}

static bool parseBoundedUInt(const String& raw, uint32_t minVal, uint32_t maxVal, uint32_t& out) {
    String v = raw;
    v.trim();
    if (v.isEmpty()) return false;
    for (size_t i = 0; i < v.length(); i++) {
        if (v[i] < '0' || v[i] > '9') {
            return false;
        }
    }
    uint32_t parsed = (uint32_t)strtoul(v.c_str(), nullptr, 10);
    if (parsed < minVal || parsed > maxVal) {
        return false;
    }
    out = parsed;
    return true;
}

static void sendJsonDocument(WebServer& server, int httpCode, const JsonDocument& doc) {
    String payload;
    serializeJson(doc, payload);
    server.send(httpCode, "application/json", payload);
}

static void sendApiError(WebServer& server, int httpCode, const char* code, const char* message) {
    DynamicJsonDocument doc(256);
    doc["status"] = "error";
    doc["code"] = code;
    doc["message"] = message;
    sendJsonDocument(server, httpCode, doc);
}

static bool isApiKeyConfigured() {
    return strlen(WC_API_KEY) > 0;
}

static bool isAuthorizedRequest(WebServer& server) {
    if (!isApiKeyConfigured()) {
        return true;
    }

    String provided = server.header("X-Api-Key");
    if (provided.isEmpty()) {
        provided = server.arg("api_key");
    }
    provided.trim();
    return provided == String(WC_API_KEY);
}

static bool requireAuthorizedRequest(WebServer& server) {
    if (isAuthorizedRequest(server)) {
        return true;
    }
    sendApiError(server, 401, "unauthorized", "API key fehlt oder ist ungueltig");
    return false;
}

static bool isHttpWriteWindowOpen() {
    return (uint64_t)esp_timer_get_time() <= HTTP_WRITE_WINDOW_US;
}

static bool requireHttpWriteWindow(WebServer& server, const char* endpointName) {
    if (isHttpWriteWindowOpen()) {
        return true;
    }

    DebugManager::print(DebugCategory::OTA, "[HTTP] Write denied after boot window: ");
    DebugManager::println(DebugCategory::OTA, endpointName);
    sendApiError(server, 403, "write_window_closed",
                 "Schreibender Endpoint nur in den ersten 60 Minuten nach Boot erlaubt");
    return false;
}

static uint32_t validUnixNow() {
    time_t now = time(nullptr);
    // Treat values before 2023 as invalid/uninitialized clock.
    if (now < 1672531200) {
        return 0;
    }
    return (uint32_t)now;
}

static bool saveConfigBackupFile(const String& ssid,
                                 const String& password,
                                 const String& mqttServer,
                                 const String& mqttUser,
                                 const String& mqttPassword,
                                 int mqttPort,
                                 const String& otaProfile,
                                 uint32_t otaSinceEpoch) {
    DynamicJsonDocument doc(768);
    doc["ssid"] = ssid;
    doc["wifi_pass"] = password;
    doc["mqtt_server"] = mqttServer;
    doc["mqtt_user"] = mqttUser;
    doc["mqtt_pass"] = mqttPassword;
    doc["mqtt_port"] = mqttPort;
    doc["ota_profile"] = otaProfile;
    doc["ota_since"] = otaSinceEpoch;

    File file = SPIFFS.open(kWiFiConfigBackupPath, "w");
    if (!file) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Backup-Datei konnte nicht geoeffnet werden");
        return false;
    }

    const size_t written = serializeJson(doc, file);
    file.close();
    if (written == 0) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Backup-Datei konnte nicht geschrieben werden");
        return false;
    }
    return true;
}

static bool loadConfigBackupFile(String& ssid,
                                 String& password,
                                 String& mqttServer,
                                 String& mqttUser,
                                 String& mqttPassword,
                                 int& mqttPort,
                                 String& otaProfile,
                                 uint32_t& otaSinceEpoch) {
    File file = SPIFFS.open(kWiFiConfigBackupPath, "r");
    if (!file) {
        return false;
    }

    DynamicJsonDocument doc(768);
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        DebugManager::print(DebugCategory::WiFi, "WiFiManager: Backup-Datei ungueltig: ");
        DebugManager::println(DebugCategory::WiFi, err.c_str());
        return false;
    }

    ssid = String((const char*)(doc["ssid"] | ""));
    password = String((const char*)(doc["wifi_pass"] | ""));
    mqttServer = String((const char*)(doc["mqtt_server"] | ""));
    mqttUser = String((const char*)(doc["mqtt_user"] | ""));
    mqttPassword = String((const char*)(doc["mqtt_pass"] | ""));
    mqttPort = doc["mqtt_port"] | 1883;
    otaProfile = normalizeOtaProfile(String((const char*)(doc["ota_profile"] | "long")));
    otaSinceEpoch = doc["ota_since"] | 0;

    return !(ssid.isEmpty() && mqttServer.isEmpty() && mqttUser.isEmpty() && mqttPassword.isEmpty());
}

extern uint8_t effectSpeed;
extern uint8_t effectIntensity;
extern uint8_t effectDensity;
extern uint16_t transitionMs;
extern StateManager& stateManager;
extern EffectManager& effectManager;
extern MQTTManager mqttManager;
extern Adafruit_NeoPixel* strip;
extern RTCManager rtcManager;
extern void handleCurrentEffect();
extern void transitionToEffect(const String& newEffect);
extern void applyControlUpdate(
    bool hasPower, bool newPower,
    bool hasBrightness, uint8_t newBrightness,
    bool hasColor, uint32_t newColor,
    bool hasEffect, const String& newEffect,
    bool debounceColor,
    bool renderNow,
    bool publishState
);

WiFiManager::WiFiManager() 
    : server(80), config_loaded(false), routes_initialized(false), setup_mode(false), setup_start_time(0), mqtt_port(1883), ota_profile("long"), ota_profile_since_epoch(0), ota_last_policy_check_ms(0) {
}

void WiFiManager::loadConfig() {
    bool prefsReadable = prefs.begin("wifi", true);
    if (!prefsReadable) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Preferences begin failed, pruefe Backup-Datei");
    } else {
        ssid = prefs.getString("ssid", "");
        password = prefs.getString("wifi_pass", "");
        mqtt_server = prefs.getString("mqtt_server", "");
        mqtt_user = prefs.getString("mqtt_user", "");
        mqtt_password = prefs.getString("mqtt_pass", "");
        mqtt_port = prefs.getInt("mqtt_port", 1883);
        ota_profile = normalizeOtaProfile(prefs.getString("ota_profile", "long"));
        ota_profile_since_epoch = (uint32_t)prefs.getULong("ota_since", 0);
        prefs.end();
    }

    // Fallback auf Backup-Datei wenn NVS nicht lesbar ODER Zugangsdaten fehlen.
    // Bedingung: SSID leer (genuegt - ohne SSID kann nicht verbunden werden).
    // Fruehere Bedingung (alle Felder leer) war zu restriktiv und hat den Fallback
    // z.B. nach einem IDF-Major-Version-Wechsel via OTA unterdrückt.
    const bool missingPrimaryConfig = ssid.isEmpty();
    if (!prefsReadable || missingPrimaryConfig) {
        String backupSsid;
        String backupPassword;
        String backupMqttServer;
        String backupMqttUser;
        String backupMqttPassword;
        int backupMqttPort = 1883;
        String backupOtaProfile = "long";
        uint32_t backupOtaSinceEpoch = 0;

        if (loadConfigBackupFile(backupSsid,
                                 backupPassword,
                                 backupMqttServer,
                                 backupMqttUser,
                                 backupMqttPassword,
                                 backupMqttPort,
                                 backupOtaProfile,
                                 backupOtaSinceEpoch)) {
            DebugManager::println(DebugCategory::WiFi, "WiFiManager: Stelle Konfiguration aus Backup-Datei wieder her");
            ssid = backupSsid;
            password = backupPassword;
            mqtt_server = backupMqttServer;
            mqtt_user = backupMqttUser;
            mqtt_password = backupMqttPassword;
            mqtt_port = backupMqttPort;
            ota_profile = backupOtaProfile;
            ota_profile_since_epoch = backupOtaSinceEpoch;

            if (prefs.begin("wifi", false)) {
                prefs.putString("ssid", ssid);
                prefs.putString("wifi_pass", password);
                prefs.putString("mqtt_server", mqtt_server);
                prefs.putString("mqtt_user", mqtt_user);
                prefs.putString("mqtt_pass", mqtt_password);
                prefs.putInt("mqtt_port", mqtt_port);
                prefs.putString("ota_profile", ota_profile);
                prefs.putULong("ota_since", ota_profile_since_epoch);
                prefs.end();
            } else {
                DebugManager::println(DebugCategory::WiFi, "WiFiManager: Wiederherstellung konnte nicht nach NVS geschrieben werden");
            }
        }
    }

    if (ota_profile_since_epoch == 0) {
        uint32_t now = validUnixNow();
        if (now > 0) {
            setOtaProfile(ota_profile, true);
        }
    }
    
    DebugManager::print(DebugCategory::WiFi, "WiFiManager: Config geladen - SSID: ");
    DebugManager::print(DebugCategory::WiFi, ssid.isEmpty() ? "(leer)" : ssid.c_str());
    DebugManager::print(DebugCategory::WiFi, ", MQTT: ");
    DebugManager::println(DebugCategory::WiFi, mqtt_server.isEmpty() ? "(leer)" : mqtt_server.c_str());
    config_loaded = true;
}

unsigned long WiFiManager::getOtaAutoCheckIntervalMs() const {
    if (ota_profile == "dev") {
        return 2UL * 60UL * 1000UL;
    }
    if (ota_profile == "norm") {
        return 12UL * 60UL * 60UL * 1000UL;
    }
    return 7UL * 24UL * 60UL * 60UL * 1000UL;
}

void WiFiManager::setOtaProfile(const String& profile, bool resetSinceEpoch) {
    ota_profile = normalizeOtaProfile(profile);
    if (resetSinceEpoch) {
        uint32_t now = validUnixNow();
        if (now > 0) {
            ota_profile_since_epoch = now;
        }
    }

    Preferences otaPrefs;
    if (!otaPrefs.begin("wifi", false)) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: OTA profile preferences begin failed");
        return;
    }

    otaPrefs.putString("ota_profile", ota_profile);
    otaPrefs.putULong("ota_since", ota_profile_since_epoch);
    otaPrefs.end();
}

void WiFiManager::refreshOtaProfilePolicy() {
    const unsigned long nowMs = millis();
    if (nowMs - ota_last_policy_check_ms < 60000UL) {
        return;
    }
    ota_last_policy_check_ms = nowMs;

    const uint32_t now = validUnixNow();
    if (now == 0) {
        return;
    }

    if (ota_profile_since_epoch == 0) {
        ota_profile_since_epoch = now;
        setOtaProfile(ota_profile, false);
        return;
    }

    if (now <= ota_profile_since_epoch) {
        return;
    }

    const uint32_t ageSec = now - ota_profile_since_epoch;
    const uint32_t oneWeekSec = 7UL * 24UL * 60UL * 60UL;
    const uint32_t twoWeeksSec = 2UL * oneWeekSec;

    if (ota_profile == "dev" && ageSec >= oneWeekSec) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Auto profile switch: dev -> norm");
        setOtaProfile("norm", true);
    } else if (ota_profile == "norm" && ageSec >= twoWeeksSec) {
        DebugManager::println(DebugCategory::OTA, "[OTA] Auto profile switch: norm -> long");
        setOtaProfile("long", true);
    }
}

void WiFiManager::saveConfig(const String& new_ssid, const String& new_password,
                             const String& new_mqtt_server, const String& new_mqtt_user,
                             const String& new_mqtt_password, int new_mqtt_port) {
    if (!prefs.begin("wifi", false)) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Preferences begin failed");
        return;
    }
    
    prefs.putString("ssid", new_ssid);
    prefs.putString("wifi_pass", new_password);
    prefs.putString("mqtt_server", new_mqtt_server);
    prefs.putString("mqtt_user", new_mqtt_user);
    prefs.putString("mqtt_pass", new_mqtt_password);
    prefs.putInt("mqtt_port", new_mqtt_port);
    
    waitMs(50);
    prefs.end();
    waitMs(100);
    
    ssid = new_ssid;
    password = new_password;
    mqtt_server = new_mqtt_server;
    mqtt_user = new_mqtt_user;
    mqtt_password = new_mqtt_password;
    mqtt_port = new_mqtt_port;

    if (!saveConfigBackupFile(ssid, password, mqtt_server, mqtt_user, mqtt_password, mqtt_port, ota_profile, ota_profile_since_epoch)) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Backup-Datei fuer Konfiguration konnte nicht aktualisiert werden");
    }
    
    DebugManager::print(DebugCategory::WiFi, "WiFiManager: Config gespeichert - SSID: ");
    DebugManager::print(DebugCategory::WiFi, new_ssid.c_str());
    DebugManager::print(DebugCategory::WiFi, ", MQTT: ");
    DebugManager::println(DebugCategory::WiFi, new_mqtt_server.c_str());
}

void WiFiManager::startSetupMode() {
    setup_mode = true;
    setup_start_time = millis();
    
    DebugManager::println(DebugCategory::WiFi, "=== WiFiManager SETUP MODE AKTIV ===");
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("WordClock-Setup");
    
    IPAddress apIP = WiFi.softAPIP();
    DebugManager::print(DebugCategory::WiFi, "AP IP: ");
    DebugManager::println(DebugCategory::WiFi, apIP);
    
    // Scan sofort starten, damit Ergebnisse beim ersten /scan-Request bereits bereit sind
    WiFi.scanNetworks(true);

    // Captive Portal DNS
    dnsServer.start(DNS_PORT, "*", apIP);
    
    setupWebRoutes();
}

void WiFiManager::stopSetupMode() {
    setup_mode = false;
    dnsServer.stop();
    DebugManager::println(DebugCategory::WiFi, "WiFiManager: Setup Mode beendet");
}

void WiFiManager::handleSetup() {
    if (setup_mode) {
        dnsServer.processNextRequest();
        server.handleClient();
        
        // Timeout nach 30 Minuten -> Neustart, damit normaler Boot erneut versucht wird
        if (millis() - setup_start_time > 30 * 60 * 1000) {
            stopSetupMode();
            rebootDevice("WiFi setup timeout", 50);
        }
    }
}

void WiFiManager::connectToWiFi() {
    if (!config_loaded) {
        loadConfig();
    }

    if (ssid.isEmpty()) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Keine SSID konfiguriert -> Setup Mode");
        startSetupMode();
        return;
    }

    String trySsid = ssid;
    String tryPass = password;
    trySsid.trim();
    tryPass.trim();
    
    DebugManager::print(DebugCategory::WiFi, "WiFiManager: Verbinde mit ");
    DebugManager::println(DebugCategory::WiFi, trySsid);
    
    WiFi.mode(WIFI_STA);
    // disconnect(true) loescht den internen WiFi-State – notwendig nach OTA-Reboot
    // mit IDF 5 (Arduino ESP32 3.x), wo begin() ohne vorherigen Reset haengen kann.
    WiFi.disconnect(true);
    waitMs(100);
    WiFi.begin(trySsid.c_str(), tryPass.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        waitMs(500);
        DebugManager::print(DebugCategory::WiFi, ".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        DebugManager::println(DebugCategory::WiFi, "\nWiFiManager: Verbunden!");
        DebugManager::print(DebugCategory::WiFi, "IP: ");
        DebugManager::println(DebugCategory::WiFi, WiFi.localIP());
        setupWebRoutes();
    } else {
        DebugManager::println(DebugCategory::WiFi, "\nWiFiManager: Verbindung fehlgeschlagen -> Setup Mode");
        startSetupMode();
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::setupWebRoutes() {
    if (routes_initialized) {
        return;
    }

    // Hauptseite mit Navigation
    server.on("/", [this]() {
        server.send(200, "text/html", home_html_page);
    });

    // Unterseiten
    server.on("/main", [this]() {
        server.send(200, "text/html", setup_html_page);
    });
    server.on("/wifi", [this]() {
        server.send(200, "text/html", setup_html_page);
    });
    server.on("/mqtt", [this]() {
        server.send(200, "text/html", setup_html_page);
    });
    server.on("/ota", [this]() {
        server.send(200, "text/html", setup_html_page);
    });
    server.on("/layout", [this]() {
        server.send(200, "text/html", setup_html_page);
    });
    server.on("/live", [this]() {
        server.send(200, "text/html", live_html_page);
    });
    server.on("/test", [this]() {
        server.send(200, "text/html", test_html_page);
    });
    
    // SSID-Scan Endpoint
    server.on("/scan", [this]() {
        handleScan();
    });
    
    // Save Endpoint
    server.on("/save", HTTP_POST, [this]() {
        handleSave();
    });

    // Runtime APIs
    server.on("/api/status", [this]() {
        handleStatus();
    });
    server.on("/api/status-lite", [this]() {
        handleStatusLite();
    });
    server.on("/api/device-name/randomize", HTTP_POST, [this]() {
        handleDeviceNameRandomize();
    });
    server.on("/api/preview", HTTP_POST, [this]() {
        handlePreview();
    });
    server.on("/api/quicktest", HTTP_POST, [this]() {
        handleQuickTest();
    });
    server.on("/api/ota/info", HTTP_GET, [this]() {
        handleOtaInfo();
    });
    server.on("/api/ota/check", HTTP_POST, [this]() {
        handleOtaCheck();
    });
    server.on("/api/ota/profile", HTTP_POST, [this]() {
        handleOtaProfile();
    });
    server.on("/api/layout", HTTP_GET, [this]() {
        handleLayoutGet();
    });
    server.on("/api/layout", HTTP_POST, [this]() {
        handleLayoutSet();
    });

    // Frontplate SVG for exact live overlay
    server.on("/ziffernblatt.svg", [this]() {
        File f = SPIFFS.open("/ziffernblatt.svg", "r");
        if (!f) {
            server.send(404, "text/plain", "ziffernblatt.svg not found");
            return;
        }
        server.streamFile(f, "image/svg+xml");
        f.close();
    });

    // Paw outline icon for minute indicators
    server.on("/Pfote.png", [this]() {
        File f = SPIFFS.open("/Pfote.png", "r");
        if (!f) {
            server.send(404, "text/plain", "Pfote.png not found");
            return;
        }
        server.streamFile(f, "image/png");
        f.close();
    });

    // Reboot Endpoint
    server.on("/reboot", [this]() {
        if (!requireAuthorizedRequest(server)) {
            return;
        }
        if (!requireHttpWriteWindow(server, "/reboot")) {
            return;
        }
        server.send(200, "text/plain", "Rebooting...");
        waitMs(500);
        rebootDevice("HTTP /reboot", 50);
    });

    // Captive Portal: iOS/Android öffnen Login-Dialog wenn unbekannte URLs auf "/" umgeleitet werden
    server.onNotFound([this]() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.begin();
    routes_initialized = true;
    DebugManager::println(DebugCategory::WiFi, "WiFiManager: Webserver-Routen aktiv");
}

void WiFiManager::handleScan() {
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_RUNNING) {
        server.send(200, "application/json", "[]");
        return;
    }

    // Deduplizieren (mehrere APs mit gleichem Namen) + nach Signalstärke sortieren
    std::vector<int> idx;
    for (int i = 0; i < n; i++) {
        String name = WiFi.SSID(i);
        if (name.isEmpty()) continue;
        bool dup = false;
        for (int j : idx) {
            if (WiFi.SSID(j) == name) { dup = true; break; }
        }
        if (!dup) idx.push_back(i);
    }
    std::sort(idx.begin(), idx.end(), [](int a, int b) {
        return WiFi.RSSI(a) > WiFi.RSSI(b);
    });

    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.to<JsonArray>();
    for (int i : idx) {
        arr.add(WiFi.SSID(i));
    }

    WiFi.scanDelete();
    WiFi.scanNetworks(true);

    String payload;
    serializeJson(arr, payload);
    server.send(200, "application/json", payload);
}

void WiFiManager::handleSave() {
    if (!requireAuthorizedRequest(server)) {
        return;
    }
    if (!requireHttpWriteWindow(server, "/save")) {
        return;
    }

    DebugManager::println(DebugCategory::WiFi, "WiFiManager: SAVE REQUEST RECEIVED");

    if (!config_loaded) {
        loadConfig();
    }
    
    String new_ssid = server.arg("ssid");
    String new_password = server.arg("wifi_pass");
    String new_mqtt_server = server.arg("mqtt_server");
    String new_mqtt_user = server.arg("mqtt_user");
    String new_mqtt_password = server.arg("mqtt_pass");
    new_ssid.trim();
    new_password.trim();
    new_mqtt_server.trim();
    new_mqtt_user.trim();
    new_mqtt_password.trim();

    // UI sends password fields optionally. Empty value must not wipe stored secrets.
    if (!server.hasArg("wifi_pass") || new_password.isEmpty()) {
        new_password = password;
    }
    if (!server.hasArg("mqtt_pass") || new_mqtt_password.isEmpty()) {
        new_mqtt_password = mqtt_password;
    }

    int new_mqtt_port = server.arg("mqtt_port").toInt();
    if (new_mqtt_port == 0) new_mqtt_port = 1883;

    // Bestimme, ob nur MQTT oder nur WiFi oder beides gespeichert werden soll
    bool has_ssid = !new_ssid.isEmpty();
    bool has_mqtt_server = !new_mqtt_server.isEmpty();
    bool has_any_mqtt_arg = has_mqtt_server || !new_mqtt_user.isEmpty() || !new_mqtt_password.isEmpty() || server.hasArg("mqtt_port");

    if (has_any_mqtt_arg) {
        if (!has_mqtt_server) {
            new_mqtt_server = mqtt_server;
        }
        if (!server.hasArg("mqtt_port") || new_mqtt_port == 0) {
            new_mqtt_port = mqtt_port;
        }
        if (!server.hasArg("mqtt_user")) {
            new_mqtt_user = mqtt_user;
        }
        if (!server.hasArg("mqtt_pass")) {
            new_mqtt_password = mqtt_password;
        }
        has_mqtt_server = !new_mqtt_server.isEmpty();
    }

    // Wenn nur MQTT Parameter, nur MQTT speichern (kein WiFi-Test)
    if (has_any_mqtt_arg && !has_ssid) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Speichern nur MQTT-Parameter");

        if (!has_mqtt_server) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"MQTT-Server fehlt\"}");
            return;
        }

        saveConfig(ssid, password, new_mqtt_server, new_mqtt_user, new_mqtt_password, new_mqtt_port);

        loadConfig();
        if (mqtt_server != new_mqtt_server || mqtt_user != new_mqtt_user ||
            mqtt_password != new_mqtt_password || mqtt_port != new_mqtt_port) {
            server.send(500, "application/json", "{\"status\":\"error\",\"msg\":\"MQTT-Speichern fehlgeschlagen\"}");
            return;
        }

        mqttManager.disconnect();
        mqttManager.setConfig(mqtt_server, mqtt_port, mqtt_user, mqtt_password);
        bool mqttTested = false;
        bool mqttConnectedNow = false;
        if (isConnected()) {
            mqttTested = true;
            mqttManager.connect();
            mqttConnectedNow = mqttManager.isConnected();
        }

        DynamicJsonDocument doc(320);
        doc["status"] = "ok";
        doc["mqtt_tested"] = mqttTested;
        doc["mqtt_connected"] = mqttConnectedNow;
        if (!mqttTested) {
            doc["msg"] = "MQTT gespeichert (WLAN offline, Test uebersprungen)";
        } else if (mqttConnectedNow) {
            doc["msg"] = "MQTT gespeichert und Verbindung erfolgreich";
        } else {
            doc["msg"] = "MQTT gespeichert, aber Verbindung fehlgeschlagen";
        }
        sendJsonDocument(server, 200, doc);
        return;
    }

    // Wenn nur WiFi Parameter: speichern + Test
    if (has_ssid && !has_mqtt_server) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Speichern nur WiFi-Parameter");

        saveConfig(new_ssid, new_password, mqtt_server, mqtt_user, mqtt_password, mqtt_port);

        // Direkt verifizieren
        loadConfig();
        if (ssid != new_ssid) {
            server.send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Speichern fehlgeschlagen\"}");
            return;
        }

        // WLAN-Verbindungstest
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Teste gespeicherte WLAN-Daten...");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        waitMs(150);
        WiFi.begin(new_ssid.c_str(), new_password.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 24) {
            waitMs(500);
            DebugManager::print(DebugCategory::WiFi, "+");
            attempts++;
        }
        DebugManager::println(DebugCategory::WiFi);

        if (WiFi.status() != WL_CONNECTED) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"WLAN-Verbindung fehlgeschlagen\"}");
            return;
        }

        server.send(200, "application/json", "{\"status\":\"ok\"}");
        return;
    }

    // Fallback: beide Parameter oder keiner - alte Logik
    if (!new_ssid.isEmpty()) {
        saveConfig(new_ssid, new_password, new_mqtt_server, new_mqtt_user, new_mqtt_password, new_mqtt_port);

        // Direkt verifizieren, dass die Daten wirklich in Preferences gelandet sind.
        loadConfig();
        if (ssid != new_ssid) {
            server.send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Speichern fehlgeschlagen\"}");
            return;
        }

        // Verbindungstest vor Reboot: bei Fehler im Setup bleiben statt Endlosschleife.
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Teste gespeicherte WLAN-Daten...");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        waitMs(150);
        WiFi.begin(new_ssid.c_str(), new_password.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 24) {
            waitMs(500);
            DebugManager::print(DebugCategory::WiFi, "+");
            attempts++;
        }
        DebugManager::println(DebugCategory::WiFi);

        if (WiFi.status() != WL_CONNECTED) {
            server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"WLAN-Verbindung fehlgeschlagen\"}");
            return;
        }

        server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"SSID fehlt\"}");
    }
}

void WiFiManager::handleStatus() {
    refreshOtaProfilePolicy();

    if (!config_loaded) {
        loadConfig();
    }

    const bool statePower = stateManager.getPowerState();
    const String stateEffect = stateManager.getCurrentEffect();
    const uint8_t stateBrightness = stateManager.getBrightness();
    const uint32_t stateColor = stateManager.getColor();
    const uint8_t stateSpeed = stateManager.getSpeed();
    const uint8_t stateIntensity = stateManager.getIntensity();
    const uint8_t stateDensity = stateManager.getDensity();
    const uint16_t stateTransitionMs = stateManager.getTransitionMs();

    // Matrix: 10 rows x 11 cols = 110 hex strings (~16 bytes/node in ArduinoJson 6) plus
    // ~40 scalar fields/string storage and small minute position metadata.
    DynamicJsonDocument doc(6400);
    doc["state"] = statePower ? "ON" : "OFF";
    doc["effect"] = stateEffect;
    doc["brightness"] = stateBrightness;
    doc["speed"] = stateSpeed;
    doc["intensity"] = stateIntensity;
    doc["density"] = stateDensity;
    doc["transition_ms"] = stateTransitionMs;

    char color_hex[8];
    snprintf(color_hex, sizeof(color_hex), "#%06lX", (unsigned long)(stateColor & 0xFFFFFF));
    doc["color"] = color_hex;
    doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "offline";
    doc["rssi"] = WiFi.isConnected() ? WiFi.RSSI() : -127;
    doc["uptime_s"] = millis() / 1000;
    doc["rtc_available"] = rtcManager.isAvailable();
    doc["rtc_temp_c"] = rtcManager.getTemperatureC();
    doc["rtc_osf"] = rtcManager.hasOscillatorStopFlag();
    doc["rtc_battery_warning"] = rtcManager.hasBatteryWarning();
    doc["rtc_temp_warning"] = rtcManager.hasTemperatureWarning();
    doc["rtc_warning"] = rtcManager.hasHealthWarning();
    doc["mqtt_connected"] = mqttManager.isConnected();
    doc["mem_free"] = MemoryManager::getFreeRam();
    doc["mem_total"] = ESP.getHeapSize();
    doc["mem_max_alloc"] = ESP.getMaxAllocHeap();
    doc["mem_min_free"] = ESP.getMinFreeHeap();
    doc["mem_level"] = MemoryManager::memoryLevelText(MemoryManager::getMemoryLevel());
    doc["mem_warning_threshold"] = MemoryManager::WARNING_THRESHOLD;
    doc["mem_critical_threshold"] = MemoryManager::CRITICAL_THRESHOLD;
    doc["ota_profile"] = ota_profile;
    doc["ota_interval_s"] = getOtaAutoCheckIntervalMs() / 1000UL;
    doc["layout_id"] = wordClockLayoutActiveId();
    doc["layout_name"] = wordClockLayoutActiveName();
    doc["layout_text"] = wordClockLayoutText();
    doc["wifi_ssid"] = ssid;
    doc["mqtt_server"] = mqtt_server;
    doc["mqtt_port"] = mqtt_port;
    doc["mqtt_user"] = mqtt_user;
    doc["device_name"] = mqttManager.getDeviceName();
    doc["device_alias"] = mqttManager.getDeviceAlias();

    JsonArray minutePositions = doc.createNestedArray("minute_positions");
    const char* minuteKeys[] = {"M1", "M2", "M3", "M4"};
    for (size_t i = 0; i < 4; i++) {
        Word w;
        if (getClockWordPosition(String(minuteKeys[i]), w)) {
            JsonObject p = minutePositions.createNestedObject();
            p["x"] = w.x;
            p["y"] = w.y;
        }
    }

    JsonArray matrix = doc.createNestedArray("matrix");
    for (int y = 0; y < HEIGHT; y++) {
        JsonArray row = matrix.createNestedArray();
        for (int x = 0; x < WIDTH; x++) {
            uint32_t c = strip->getPixelColor(XY(x, y));
            // Export canonical RGB hex (#RRGGBB) for the web UI.
            uint8_t r = (uint8_t)((c >> 16) & 0xFF);
            uint8_t g = (uint8_t)((c >> 8) & 0xFF);
            uint8_t b = (uint8_t)(c & 0xFF);
            char hex[7];
            snprintf(hex, sizeof(hex), "%02X%02X%02X",
                (unsigned int)r,
                (unsigned int)g,
                (unsigned int)b);
            row.add(hex);
        }
    }

    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handleStatusLite() {
    refreshOtaProfilePolicy();

    if (!config_loaded) {
        loadConfig();
    }

    const bool statePower = stateManager.getPowerState();
    const String stateEffect = stateManager.getCurrentEffect();
    const uint8_t stateBrightness = stateManager.getBrightness();
    const uint32_t stateColor = stateManager.getColor();
    const uint8_t stateSpeed = stateManager.getSpeed();
    const uint8_t stateIntensity = stateManager.getIntensity();
    const uint8_t stateDensity = stateManager.getDensity();
    const uint16_t stateTransitionMs = stateManager.getTransitionMs();

    DynamicJsonDocument doc(2048);
    doc["state"] = statePower ? "ON" : "OFF";
    doc["effect"] = stateEffect;
    doc["brightness"] = stateBrightness;
    doc["speed"] = stateSpeed;
    doc["intensity"] = stateIntensity;
    doc["density"] = stateDensity;
    doc["transition_ms"] = stateTransitionMs;

    char color_hex[8];
    snprintf(color_hex, sizeof(color_hex), "#%06lX", (unsigned long)(stateColor & 0xFFFFFF));
    doc["color"] = color_hex;
    doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "offline";
    doc["rssi"] = WiFi.isConnected() ? WiFi.RSSI() : -127;
    doc["uptime_s"] = millis() / 1000;
    doc["rtc_available"] = rtcManager.isAvailable();
    doc["rtc_temp_c"] = rtcManager.getTemperatureC();
    doc["rtc_osf"] = rtcManager.hasOscillatorStopFlag();
    doc["rtc_battery_warning"] = rtcManager.hasBatteryWarning();
    doc["rtc_temp_warning"] = rtcManager.hasTemperatureWarning();
    doc["rtc_warning"] = rtcManager.hasHealthWarning();
    doc["mqtt_connected"] = mqttManager.isConnected();
    doc["mem_free"] = MemoryManager::getFreeRam();
    doc["mem_total"] = ESP.getHeapSize();
    doc["mem_max_alloc"] = ESP.getMaxAllocHeap();
    doc["mem_min_free"] = ESP.getMinFreeHeap();
    doc["mem_level"] = MemoryManager::memoryLevelText(MemoryManager::getMemoryLevel());
    doc["mem_warning_threshold"] = MemoryManager::WARNING_THRESHOLD;
    doc["mem_critical_threshold"] = MemoryManager::CRITICAL_THRESHOLD;
    doc["ota_profile"] = ota_profile;
    doc["ota_interval_s"] = getOtaAutoCheckIntervalMs() / 1000UL;
    doc["layout_id"] = wordClockLayoutActiveId();
    doc["layout_name"] = wordClockLayoutActiveName();
    doc["layout_text"] = wordClockLayoutText();
    doc["wifi_ssid"] = ssid;
    doc["mqtt_server"] = mqtt_server;
    doc["mqtt_port"] = mqtt_port;
    doc["mqtt_user"] = mqtt_user;
    doc["device_name"] = mqttManager.getDeviceName();
    doc["device_alias"] = mqttManager.getDeviceAlias();

    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handleDeviceNameRandomize() {
    if (!mqttManager.isConnected()) {
        sendApiError(server, 409, "mqtt_not_connected",
                     "Namensaenderung braucht eine aktive MQTT-Verbindung");
        return;
    }

    const String previousName = mqttManager.getDeviceName();
    if (!mqttManager.randomizeDeviceName()) {
        sendApiError(server, 500, "device_name_failed",
                     "Freier Geraetename konnte nicht gespeichert werden");
        return;
    }

    DynamicJsonDocument doc(256);
    doc["status"] = "ok";
    doc["device_name"] = mqttManager.getDeviceName();
    doc["device_alias"] = mqttManager.getDeviceAlias();
    doc["changed"] = (previousName != mqttManager.getDeviceName());
    doc["message"] = (previousName != mqttManager.getDeviceName())
        ? "Neuer Geraetename gesetzt"
        : "Geraetename bestaetigt";
    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handlePreview() {
    if (!requireAuthorizedRequest(server)) {
        return;
    }
    if (!requireHttpWriteWindow(server, "/api/preview")) {
        return;
    }

    String stateArg = server.arg("state");
    String effectArg = server.arg("effect");
    String colorArg = server.arg("color");
    String brightnessArg = server.arg("brightness");
    String speedArg = server.arg("speed");
    String intensityArg = server.arg("intensity");
    String densityArg = server.arg("density");
    String transitionArg = server.arg("transition_ms");

    stateArg.trim();
    effectArg.trim();
    colorArg.trim();
    brightnessArg.trim();
    speedArg.trim();
    intensityArg.trim();
    densityArg.trim();
    transitionArg.trim();

    DebugManager::print(DebugCategory::WiFi, "Live preview request: state=");
    DebugManager::print(DebugCategory::WiFi, stateArg);
    DebugManager::print(DebugCategory::WiFi, " effect=");
    DebugManager::print(DebugCategory::WiFi, effectArg);
    DebugManager::print(DebugCategory::WiFi, " color=");
    DebugManager::print(DebugCategory::WiFi, colorArg);
    DebugManager::print(DebugCategory::WiFi, " brightness=");
    DebugManager::println(DebugCategory::WiFi, brightnessArg);

    bool tuningChanged = false;
    const bool hasAnyArg = !stateArg.isEmpty() || !effectArg.isEmpty() || !colorArg.isEmpty() ||
                           !brightnessArg.isEmpty() || !speedArg.isEmpty() || !intensityArg.isEmpty() ||
                           !densityArg.isEmpty() || !transitionArg.isEmpty();
    if (!hasAnyArg) {
        sendApiError(server, 400, "invalid_request", "Keine Parameter uebergeben");
        return;
    }

    bool hasPower = !stateArg.isEmpty();
    bool newPower = stateManager.getPowerState();
    if (hasPower) {
        if (stateArg != "ON" && stateArg != "OFF") {
            sendApiError(server, 422, "invalid_state", "state muss ON oder OFF sein");
            return;
        }
        newPower = (stateArg == "ON");
    }

    bool hasBrightness = !brightnessArg.isEmpty();
    uint8_t newBrightness = stateManager.getBrightness();
    if (hasBrightness) {
        uint32_t b = 0;
        if (!parseBoundedUInt(brightnessArg, 0, 255, b)) {
            sendApiError(server, 422, "invalid_brightness", "brightness muss zwischen 0 und 255 liegen");
            return;
        }
        newBrightness = (uint8_t)b;
    }

    bool hasColor = false;
    uint32_t newColor = stateManager.getColor();
    if (!colorArg.isEmpty()) {
        if (colorArg.startsWith("#")) {
            colorArg = colorArg.substring(1);
        }
        if (!isHexColor6(colorArg)) {
            sendApiError(server, 422, "invalid_color", "color muss #RRGGBB sein");
            return;
        }
        uint32_t parsed = (uint32_t)strtoul(colorArg.c_str(), nullptr, 16);
        newColor = parsed & 0xFFFFFF;
        hasColor = true;
    }

    bool hasEffect = !effectArg.isEmpty();
    if (hasEffect) {
        if (effectArg != "clock" && effectManager.getEffect(effectArg) == nullptr) {
            sendApiError(server, 422, "invalid_effect", "Unbekannter Effekt");
            return;
        }
    }

    if (!speedArg.isEmpty()) {
        uint32_t value = 0;
        if (!parseBoundedUInt(speedArg, ControlConfig::SPEED_MIN, ControlConfig::SPEED_MAX, value)) {
            sendApiError(server, 422, "invalid_speed", "speed ausserhalb gueltigem Bereich");
            return;
        }
        effectSpeed = (uint8_t)value;
        stateManager.setSpeed(effectSpeed);
        tuningChanged = true;
    }
    if (!intensityArg.isEmpty()) {
        uint32_t value = 0;
        if (!parseBoundedUInt(intensityArg, ControlConfig::INTENSITY_MIN, ControlConfig::INTENSITY_MAX, value)) {
            sendApiError(server, 422, "invalid_intensity", "intensity ausserhalb gueltigem Bereich");
            return;
        }
        effectIntensity = (uint8_t)value;
        stateManager.setIntensity(effectIntensity);
        tuningChanged = true;
    }
    if (!densityArg.isEmpty()) {
        uint32_t value = 0;
        if (!parseBoundedUInt(densityArg, ControlConfig::DENSITY_MIN, ControlConfig::DENSITY_MAX, value)) {
            sendApiError(server, 422, "invalid_density", "density ausserhalb gueltigem Bereich");
            return;
        }
        effectDensity = (uint8_t)value;
        stateManager.setDensity(effectDensity);
        tuningChanged = true;
    }
    if (!transitionArg.isEmpty()) {
        uint32_t value = 0;
        if (!parseBoundedUInt(transitionArg, ControlConfig::TRANSITION_MIN_MS, ControlConfig::TRANSITION_MAX_MS, value)) {
            sendApiError(server, 422, "invalid_transition", "transition_ms ausserhalb gueltigem Bereich");
            return;
        }
        transitionMs = (uint16_t)value;
        stateManager.setTransitionMs(transitionMs);
        tuningChanged = true;
    }

    applyControlUpdate(
        hasPower, newPower,
        hasBrightness, newBrightness,
        hasColor, newColor,
        hasEffect, effectArg,
        false,  // web preview should react instantly
        true,   // render updates
        true    // publish resulting state
    );

    if (tuningChanged) {
        stateManager.scheduleSave();
        mqttManager.publishTuningState();
    }

    handleStatus();
}

void WiFiManager::handleQuickTest() {
    if (!requireAuthorizedRequest(server)) {
        return;
    }
    if (!requireHttpWriteWindow(server, "/api/quicktest")) {
        return;
    }

    String action = server.arg("action");
    action.trim();
    if (action.isEmpty()) {
        sendApiError(server, 400, "invalid_request", "action fehlt");
        return;
    }

    static const char* kAllowedActions[] = {
        "all_on", "all_off", "clock_test", "gradient",
        "color_red", "color_green", "color_blue", "color_yellow", "color_cyan", "color_magenta",
        "brightness_0", "brightness_25", "brightness_50", "brightness_75", "brightness_100",
        "rainbow", "blink", "pulse", "spiral",
        "checker", "rows", "columns", "warm_white", "cool_white", "sparkle"
    };
    bool actionAllowed = false;
    for (size_t i = 0; i < (sizeof(kAllowedActions) / sizeof(kAllowedActions[0])); i++) {
        if (action == kAllowedActions[i]) {
            actionAllowed = true;
            break;
        }
    }
    if (!actionAllowed) {
        sendApiError(server, 422, "invalid_action", "Unbekannte quicktest action");
        return;
    }

    uint32_t holdMs = 0;
    String holdRaw = server.arg("hold_ms");
    if (!holdRaw.isEmpty()) {
        if (!parseBoundedUInt(holdRaw, 0, 10000, holdMs)) {
            sendApiError(server, 422, "invalid_hold_ms", "hold_ms muss zwischen 0 und 10000 liegen");
            return;
        }
    }

    if (action == "all_on") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                const uint32_t activeColor = stateManager.getColor();
                uint8_t r = (activeColor >> 16) & 0xFF;
                uint8_t g = (activeColor >> 8) & 0xFF;
                uint8_t b = activeColor & 0xFF;
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
            }
        }
        strip->show();
    } else if (action == "all_off") {
        clearMatrix();
        strip->show();
    } else if (action == "clock_test") {
        struct tm t;
        if (getLocalTime(&t)) {
            showTime(t.tm_hour, t.tm_min);
        } else {
            showTime(12, 0);
        }
    } else if (action == "gradient") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint8_t r = (uint8_t)((x * 255) / max(1, WIDTH - 1));
                uint8_t g = (uint8_t)((y * 255) / max(1, HEIGHT - 1));
                uint8_t b = 255 - r;
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
            }
        }
        strip->show();
    }
    // Color tests
    else if (action == "color_red") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 0, 0));
            }
        }
        strip->show();
    } else if (action == "color_green") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(0, 255, 0));
            }
        }
        strip->show();
    } else if (action == "color_blue") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(0, 0, 255));
            }
        }
        strip->show();
    } else if (action == "color_yellow") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 255, 0));
            }
        }
        strip->show();
    } else if (action == "color_cyan") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(0, 255, 255));
            }
        }
        strip->show();
    } else if (action == "color_magenta") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 0, 255));
            }
        }
        strip->show();
    }
    // Brightness tests (using white)
    else if (action == "brightness_0") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), 0);
            }
        }
        strip->show();
    } else if (action == "brightness_25") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), strip->Color(64, 64, 64));
            }
        }
        strip->show();
    } else if (action == "brightness_50") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 255, 255));
            }
        }
        strip->show();
    } else if (action == "brightness_75") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), strip->Color(192, 192, 192));
            }
        }
        strip->show();
    } else if (action == "brightness_100") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 255, 255));
            }
        }
        strip->show();
    }
    // Special tests
    else if (action == "rainbow") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint16_t hue = (uint16_t)(((x + y) * 65535) / (WIDTH + HEIGHT));
                uint32_t c = strip->ColorHSV(hue, 255, 255);
                strip->setPixelColor(XY(x, y), c);
            }
        }
        strip->show();
    } else if (action == "blink") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 255, 255));
            }
        }
        strip->show();
        delay(300);
        clearMatrix();
        strip->show();
        delay(300);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 255, 255));
            }
        }
        strip->show();
    } else if (action == "pulse") {
        for (int step = 0; step < 20; step++) {
            uint8_t bright = (step < 10) ? (step * 25) : ((19 - step) * 25);
            for (int y = 0; y < HEIGHT; y++) {
                for (int x = 0; x < WIDTH; x++) {
                    strip->setPixelColor(XY(x, y), strip->Color(bright, 0, 0));
                }
            }
            strip->show();
            delay(30);
        }
    } else if (action == "spiral") {
        for (int step = 0; step < 12; step++) {
            clearMatrix();
            for (int y = 0; y < HEIGHT; y++) {
                for (int x = 0; x < WIDTH; x++) {
                    int dist = abs(x - WIDTH/2) + abs(y - HEIGHT/2);
                    if (dist % 3 == step % 3) {
                        strip->setPixelColor(XY(x, y), makeColorWithBrightness(0, 255, 255));
                    }
                }
            }
            strip->show();
            delay(150);
        }
    } else if (action == "checker") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                bool even = ((x + y) % 2) == 0;
                strip->setPixelColor(XY(x, y), even ? makeColorWithBrightness(255, 140, 0) : makeColorWithBrightness(55, 75, 110));
            }
        }
        strip->show();
    } else if (action == "rows") {
        for (int y = 0; y < HEIGHT; y++) {
            uint8_t r = (y % 2 == 0) ? 255 : 40;
            uint8_t g = (y % 2 == 0) ? 190 : 70;
            uint8_t b = (y % 2 == 0) ? 70 : 130;
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
            }
        }
        strip->show();
    } else if (action == "columns") {
        for (int x = 0; x < WIDTH; x++) {
            uint8_t r = (x % 2 == 0) ? 120 : 35;
            uint8_t g = (x % 2 == 0) ? 150 : 95;
            uint8_t b = (x % 2 == 0) ? 255 : 80;
            for (int y = 0; y < HEIGHT; y++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
            }
        }
        strip->show();
    } else if (action == "warm_white") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 170, 95));
            }
        }
        strip->show();
    } else if (action == "cool_white") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(165, 210, 255));
            }
        }
        strip->show();
    } else if (action == "sparkle") {
        clearMatrix();
        for (int i = 0; i < 45; i++) {
            int x = random(WIDTH);
            int y = random(HEIGHT);
            strip->setPixelColor(XY(x, y), makeColorWithBrightness(255, 255, 255));
            strip->show();
            delay(28);
            strip->setPixelColor(XY(x, y), 0);
        }
        strip->show();
    }

    if (holdMs > 0) {
        waitMs(holdMs);
    }

    DynamicJsonDocument doc(160);
    doc["status"] = "ok";
    doc["action"] = action;
    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handleOtaInfo() {
    refreshOtaProfilePolicy();

    DynamicJsonDocument doc(512);
    doc["status"] = "ok";
    doc["fw_version"] = getFirmwareVersion();
    doc["wifi_connected"] = WiFi.isConnected();
    doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "offline";
    doc["ota_profile"] = ota_profile;
    doc["ota_channel"] = getOtaChannel();
    doc["ota_interval_s"] = getOtaAutoCheckIntervalMs() / 1000UL;

    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handleOtaCheck() {
    if (!requireAuthorizedRequest(server)) {
        return;
    }

    DynamicJsonDocument doc(384);
    doc["status"] = "ok";
    doc["fw_version"] = getFirmwareVersion();

    if (!WiFi.isConnected()) {
        sendApiError(server, 400, "wifi_offline", "Kein WLAN verbunden");
        return;
    }

    bool started = checkForUpdateAndInstall(true);
    doc["update_started"] = started;
    doc["message"] = started ? "Update gestartet" : "Keine neuere Version gefunden";
    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handleOtaProfile() {
    if (!requireAuthorizedRequest(server)) {
        return;
    }
    if (!requireHttpWriteWindow(server, "/api/ota/profile")) {
        return;
    }

    String profile = server.arg("profile");
    profile.trim();
    profile.toLowerCase();
    if (!(profile == "dev" || profile == "norm" || profile == "long")) {
        sendApiError(server, 422, "invalid_profile", "profile muss dev, norm oder long sein");
        return;
    }
    setOtaProfile(profile, true);

    DynamicJsonDocument doc(384);
    doc["status"] = "ok";
    doc["ota_profile"] = ota_profile;
    doc["ota_interval_s"] = getOtaAutoCheckIntervalMs() / 1000UL;
    doc["ota_channel"] = getOtaChannel();
    doc["message"] = "OTA-Profil gespeichert";
    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handleLayoutGet() {
    DynamicJsonDocument doc(4096);
    doc["status"] = "ok";
    doc["layout_id"] = wordClockLayoutActiveId();
    doc["layout_name"] = wordClockLayoutActiveName();
    doc["layout_text"] = wordClockLayoutText();
    doc["word_positions"] = wordClockLayoutWordPositionsJson();
    sendJsonDocument(server, 200, doc);
}

void WiFiManager::handleLayoutSet() {
    if (!requireAuthorizedRequest(server)) {
        return;
    }
    if (!requireHttpWriteWindow(server, "/api/layout")) {
        return;
    }

    String layoutId = server.arg("layout_id");
    String layoutName = server.arg("layout_name");
    String layoutText = server.arg("layout_text");
    String wordPositions = server.arg("word_positions");
    layoutId.trim();

    if (layoutId.isEmpty()) {
        sendApiError(server, 400, "invalid_request", "layout_id fehlt");
        return;
    }

    String error;
    if (!wordClockLayoutApplyAndStore(layoutId, layoutName, layoutText, wordPositions, error)) {
        sendApiError(server, 422, "invalid_layout", error.c_str());
        return;
    }

    DynamicJsonDocument doc(1024);
    doc["status"] = "ok";
    doc["layout_id"] = wordClockLayoutActiveId();
    doc["layout_name"] = wordClockLayoutActiveName();
    doc["layout_text"] = wordClockLayoutText();
    doc["message"] = "Layout gespeichert";
    sendJsonDocument(server, 200, doc);
}
