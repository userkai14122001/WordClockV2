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
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
#include <time.h>

static const byte DNS_PORT = 53;

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

static uint32_t validUnixNow() {
    time_t now = time(nullptr);
    // Treat values before 2023 as invalid/uninitialized clock.
    if (now < 1672531200) {
        return 0;
    }
    return (uint32_t)now;
}

extern bool powerState;
extern String currentEffect;
extern uint32_t color;
extern uint8_t brightness;
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
    if (!prefs.begin("wifi", true)) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Preferences begin failed");
        return;
    }
    
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("wifi_pass", "");
    mqtt_server = prefs.getString("mqtt_server", "");
    mqtt_user = prefs.getString("mqtt_user", "");
    mqtt_password = prefs.getString("mqtt_pass", "");
    mqtt_port = prefs.getInt("mqtt_port", 1883);
    ota_profile = normalizeOtaProfile(prefs.getString("ota_profile", "long"));
    ota_profile_since_epoch = (uint32_t)prefs.getULong("ota_since", 0);
    
    prefs.end();

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
    WiFi.begin(trySsid.c_str(), tryPass.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
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

    String json;
    serializeJson(arr, json);
    server.send(200, "application/json", json);
}

void WiFiManager::handleSave() {
    DebugManager::println(DebugCategory::WiFi, "WiFiManager: SAVE REQUEST RECEIVED");
    
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
    int new_mqtt_port = server.arg("mqtt_port").toInt();
    if (new_mqtt_port == 0) new_mqtt_port = 1883;

    // Bestimme, ob nur MQTT oder nur WiFi oder beides gespeichert werden soll
    bool has_ssid = !new_ssid.isEmpty();
    bool has_mqtt_server = !new_mqtt_server.isEmpty();

    // Wenn nur MQTT Parameter, nur MQTT speichern (kein WiFi-Test)
    if (has_mqtt_server && !has_ssid) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Speichern nur MQTT-Parameter");
        Preferences prefs;
        if (!prefs.begin("wordclock", false)) {
            server.send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Speichern fehlgeschlagen\"}");
            return;
        }
        prefs.putString("mqtt_server", new_mqtt_server);
        prefs.putInt("mqtt_port", new_mqtt_port);
        prefs.putString("mqtt_user", new_mqtt_user);
        prefs.putString("mqtt_pass", new_mqtt_password);
        prefs.end();

        // Neuladen + einfache Bestätigung
        loadConfig();
        server.send(200, "application/json", "{\"status\":\"ok\"}");
        return;
    }

    // Wenn nur WiFi Parameter: speichern + Test
    if (has_ssid && !has_mqtt_server) {
        DebugManager::println(DebugCategory::WiFi, "WiFiManager: Speichern nur WiFi-Parameter");
        
        // Benutze aktuelle MQTT-Konfiguration
        String current_mqtt_server = String("");
        String current_mqtt_user = String("");
        String current_mqtt_password = String("");
        int current_mqtt_port = 1883;

        Preferences prefs;
        if (prefs.begin("wordclock", true)) {
            current_mqtt_server = prefs.getString("mqtt_server", "");
            current_mqtt_user = prefs.getString("mqtt_user", "");
            current_mqtt_password = prefs.getString("mqtt_pass", "");
            current_mqtt_port = prefs.getInt("mqtt_port", 1883);
            prefs.end();
        }

        saveConfig(new_ssid, new_password, current_mqtt_server, current_mqtt_user, current_mqtt_password, current_mqtt_port);

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

    DynamicJsonDocument doc(4096);
    doc["state"] = powerState ? "ON" : "OFF";
    doc["effect"] = currentEffect;
    doc["brightness"] = brightness;
    doc["speed"] = effectSpeed;
    doc["intensity"] = effectIntensity;
    doc["density"] = effectDensity;
    doc["transition_ms"] = transitionMs;

    char color_hex[8];
    snprintf(color_hex, sizeof(color_hex), "#%06lX", (unsigned long)(color & 0xFFFFFF));
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

    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
}

void WiFiManager::handlePreview() {
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

    bool hasPower = !stateArg.isEmpty();
    bool newPower = hasPower ? (stateArg == "ON") : powerState;

    bool hasBrightness = !brightnessArg.isEmpty();
    uint8_t newBrightness = brightness;
    if (hasBrightness) {
        int b = brightnessArg.toInt();
        if (b < 0) b = 0;
        if (b > 255) b = 255;
        newBrightness = (uint8_t)b;
    }

    bool hasColor = false;
    uint32_t newColor = color;
    if (!colorArg.isEmpty()) {
        if (colorArg.startsWith("#")) {
            colorArg = colorArg.substring(1);
        }
        if (colorArg.length() == 6) {
            uint32_t parsed = (uint32_t)strtoul(colorArg.c_str(), nullptr, 16);
            newColor = parsed & 0xFFFFFF;
            hasColor = true;
        }
    }

    bool hasEffect = !effectArg.isEmpty();

    if (!speedArg.isEmpty()) {
        effectSpeed = (uint8_t)constrain(speedArg.toInt(), (int)ControlConfig::SPEED_MIN, (int)ControlConfig::SPEED_MAX);
        stateManager.setSpeed(effectSpeed);
        tuningChanged = true;
    }
    if (!intensityArg.isEmpty()) {
        effectIntensity = (uint8_t)constrain(intensityArg.toInt(), (int)ControlConfig::INTENSITY_MIN, (int)ControlConfig::INTENSITY_MAX);
        stateManager.setIntensity(effectIntensity);
        tuningChanged = true;
    }
    if (!densityArg.isEmpty()) {
        effectDensity = (uint8_t)constrain(densityArg.toInt(), (int)ControlConfig::DENSITY_MIN, (int)ControlConfig::DENSITY_MAX);
        stateManager.setDensity(effectDensity);
        tuningChanged = true;
    }
    if (!transitionArg.isEmpty()) {
        transitionMs = (uint16_t)constrain(transitionArg.toInt(), (int)ControlConfig::TRANSITION_MIN_MS, (int)ControlConfig::TRANSITION_MAX_MS);
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
    String action = server.arg("action");
    action.trim();
    int holdMs = server.arg("hold_ms").toInt();
    holdMs = constrain(holdMs, 0, 10000);

    if (action == "all_on") {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t g = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;
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
        waitMs((uint32_t)holdMs);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WiFiManager::handleOtaInfo() {
    refreshOtaProfilePolicy();

    DynamicJsonDocument doc(512);
    doc["status"] = "ok";
    doc["fw_version"] = getFirmwareVersion();
    doc["wifi_connected"] = WiFi.isConnected();
    doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "offline";
    doc["ota_profile"] = ota_profile;
    doc["ota_interval_s"] = getOtaAutoCheckIntervalMs() / 1000UL;

    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
}

void WiFiManager::handleOtaCheck() {
    DynamicJsonDocument doc(384);
    doc["status"] = "ok";
    doc["fw_version"] = getFirmwareVersion();

    if (!WiFi.isConnected()) {
        doc["status"] = "error";
        doc["message"] = "Kein WLAN verbunden";
        String payload;
        serializeJson(doc, payload);
        server.send(400, "application/json", payload);
        return;
    }

    bool started = checkForUpdateAndInstall(true);
    doc["update_started"] = started;
    doc["message"] = started ? "Update gestartet" : "Keine neuere Version gefunden";

    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
}

void WiFiManager::handleOtaProfile() {
    String profile = server.arg("profile");
    setOtaProfile(profile, true);

    DynamicJsonDocument doc(384);
    doc["status"] = "ok";
    doc["ota_profile"] = ota_profile;
    doc["ota_interval_s"] = getOtaAutoCheckIntervalMs() / 1000UL;
    doc["message"] = "OTA-Profil gespeichert";

    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
}
