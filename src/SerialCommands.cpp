#include <Arduino.h>
#include <WiFi.h>
#include <vector>

#include "SerialCommands.h"
#include "WiFiManager.h"
#include "RTCManager.h"
#include "StateManager.h"
#include "EffectManager.h"
#include "ControlConfig.h"
#include "DebugManager.h"
#include "MemoryManager.h"
#include "SystemControl.h"
#include "ota_https_update.h"
#include "MQTTManager.h"
#include "effects.h"
#include "WordClockLayout.h"

// External runtime state
extern bool powerState;
extern String currentEffect;
extern uint32_t color;
extern uint8_t brightness;
extern uint8_t effectSpeed;
extern uint8_t effectIntensity;
extern uint16_t transitionMs;

// External managers
extern WiFiManager wifiManager;
extern RTCManager rtcManager;
extern StateManager& stateManager;
extern EffectManager& effectManager;
extern MQTTManager mqttManager;

// External functions implemented in main.cpp
extern void runFullTimeTest();
extern void runSmokeTest();
extern void runSelfTest();
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

namespace {
    enum class MatchMode {
        Exact,
        Prefix
    };

    using CommandHandler = bool (*)(const String& args);

    struct CommandDef {
        const char* trigger;
        MatchMode mode;
        const char* usage;
        const char* description;
        CommandHandler handler;
        bool showInHelp;
    };

    static inline void waitMs(uint32_t ms) {
        unsigned long start = millis();
        while (millis() - start < ms) {
            yield();
        }
    }

    static bool parseHourMinuteInput(String input, int& hour, int& minute) {
        input.trim();
        hour = -1;
        minute = -1;

        if (input.indexOf(':') > 0) {
            int sep = input.indexOf(':');
            hour = input.substring(0, sep).toInt();
            minute = input.substring(sep + 1).toInt();
        } else {
            int space = input.indexOf(' ');
            if (space > 0) {
                hour = input.substring(0, space).toInt();
                minute = input.substring(space + 1).toInt();
            }
        }

        return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
    }

    static bool parseRgbInput(String args, int& r, int& g, int& b) {
        args.trim();
        int s1 = args.indexOf(' ');
        int s2 = args.indexOf(' ', s1 + 1);
        if (s1 < 0 || s2 < 0) {
            return false;
        }

        r = args.substring(0, s1).toInt();
        g = args.substring(s1 + 1, s2).toInt();
        b = args.substring(s2 + 1).toInt();
        return r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255;
    }

    static bool equalsIgnoreCase(const String& a, const char* b) {
        String left = a;
        String right = String(b);
        left.toLowerCase();
        right.toLowerCase();
        return left == right;
    }

    static bool startsWithIgnoreCase(const String& value, const char* prefix) {
        String left = value;
        String p = String(prefix);
        left.toLowerCase();
        p.toLowerCase();
        return left.startsWith(p);
    }

    static std::vector<String> parseQuotedTokens(const String& input) {
        std::vector<String> tokens;
        bool inQuotes = false;
        String current = "";

        for (int i = 0; i < input.length(); i++) {
            char c = input[i];
            if (c == '"') {
                inQuotes = !inQuotes;
                continue;
            }

            if (c == ' ' && !inQuotes) {
                if (current.length() > 0) {
                    tokens.push_back(current);
                    current = "";
                }
            } else {
                current += c;
            }
        }

        if (current.length() > 0) {
            tokens.push_back(current);
        }
        return tokens;
    }

    static bool cmdRtc(const String&) {
        if (!rtcManager.isAvailable()) {
            Serial.println("RTC: nicht verfuegbar");
            return true;
        }

        DateTime now = rtcManager.getTime();
        Serial.printf(
            "RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second()
        );
        return true;
    }

    static bool cmdReboot(const String&) {
        Serial.println("Starte neu...");
        rebootDevice("Serial reboot command", 200);
        return true;
    }

    static bool cmdCreds(const String&) {
        Serial.println("=== STORED CREDS ===");
        Serial.println("SSID: " + wifiManager.getSSID());
        Serial.println("WIFI PASS: " + wifiManager.getPassword());
        Serial.println("MQTT SERVER: " + wifiManager.getMQTTServer());
        Serial.println("MQTT USER: " + wifiManager.getMQTTUser());
        Serial.println("MQTT PASS: " + wifiManager.getMQTTPassword());
        Serial.println("MQTT PORT: " + String(wifiManager.getMQTTPort()));
        return true;
    }

    static bool cmdCredsFlush(const String&) {
        Serial.println("===== WLAN Credentials loeschen =====");
        wifiManager.saveConfig("", "", "", "", "", 1883);
        Serial.println("Credentials geloescht. Neustart...");
        rebootDevice("Serial creds flush", 500);
        return true;
    }

    static bool cmdSmoke(const String&) {
        runSmokeTest();
        return true;
    }

    static bool cmdSelftest(const String&) {
        runSelfTest();
        return true;
    }

    static bool cmdStatus(const String&) {
        Serial.println("============ STATUS ============");
        Serial.printf("Power:       %s\n", powerState ? "ON" : "OFF");
        Serial.printf("Effekt:      %s\n", currentEffect.c_str());
        Serial.printf("Helligkeit:  %d\n", brightness);
        Serial.printf("Farbe:       0x%06X\n", color);
        Serial.printf("WiFi:        %s\n", WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "nicht verbunden");
        Serial.printf("Setup-Mode:  %s\n", wifiManager.isSetupMode() ? "JA" : "nein");
        Serial.printf("SSID:        %s\n", wifiManager.getSSID().isEmpty() ? "(leer)" : wifiManager.getSSID().c_str());
        Serial.printf("MQTT:        %s\n", mqttManager.isConnected() ? "Verbunden" : "Getrennt");
        if (!wifiManager.getMQTTServer().isEmpty()) {
            Serial.printf("MQTT-Server: %s:%d\n", wifiManager.getMQTTServer().c_str(), wifiManager.getMQTTPort());
        }
        if (rtcManager.isAvailable()) {
            DateTime now = rtcManager.getTime();
            Serial.printf("RTC:         %04d-%02d-%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
        } else {
            Serial.println("RTC:         nicht verfuegbar");
        }
        Serial.printf("Uptime:      %lus\n", millis() / 1000);
        Serial.printf("Layout:      %s (%s)\n", wordClockLayoutActiveName().c_str(), wordClockLayoutActiveId().c_str());

        uint32_t freeRam = MemoryManager::getFreeRam();
        uint32_t totalRam = ESP.getHeapSize();
        float usedRamPct = 100.0f * (totalRam - freeRam) / totalRam;
        Serial.printf("Memory:      %.1f%% used (%u / %u bytes)\n", usedRamPct, totalRam - freeRam, totalRam);
        Serial.printf("             %s\n", MemoryManager::memoryLevelText(MemoryManager::getMemoryLevel()));

        uint32_t flashTotal = ESP.getFlashChipSize();
        uint32_t sketchSize = ESP.getSketchSize();
        uint32_t sketchFree = ESP.getFreeSketchSpace();
        float usedFlashPct = 100.0f * sketchSize / flashTotal;
        Serial.printf("Flash:       %.1f%% used (%u / %u bytes)\n", usedFlashPct, sketchSize, flashTotal);
        Serial.printf("             %u bytes frei\n", sketchFree);

        Serial.println("================================");
        return true;
    }

    static bool cmdDiag(const String&) {
        MemoryManager::printMemoryStats();
        Serial.println("Heavy Effects: fire2d, matrix, plasma");
        Serial.println("Lazy-Alloc Effects: BouncingBalls (110B), Fire2D (110B), MatrixRain (90B)");
        return true;
    }

    static bool cmdSetup(const String&) {
        Serial.println("Setup-Modus wird gestartet...");
        wifiManager.startSetupMode();
        return true;
    }

    static bool cmdOtaInfo(const String&) {
        Serial.println("=== OTA Info ===");
        Serial.printf("Version lokal : %s\n", getFirmwareVersion());
        Serial.printf("Kanal         : %s\n", getOtaChannel());
        Serial.printf("OTA-Profil    : %s\n", wifiManager.getOtaProfile().c_str());
        Serial.printf("Uptime        : %lu s\n", millis() / 1000UL);
        Serial.println("Tipp: 'OTA Check' um nach Updates zu suchen");
        return true;
    }

    static bool cmdUpdateCheck(const String&) {
        Serial.printf("Pruefe auf neue Firmware-Version (lokal: %s)...\n", getFirmwareVersion());
        bool started = checkForUpdateAndInstall(true);
        if (!started) {
            Serial.println("Kein Update verfuegbar oder Pruefung fehlgeschlagen.");
        }
        return true;
    }

    static bool cmdDebugLayout(const String&) {
        runFullTimeTest();
        return true;
    }

    static bool cmdDebug(const String& args) {
        String mode = args;
        mode.trim();
        mode.toLowerCase();

        if (mode.isEmpty() || mode == "help") {
            DebugManager::printHelp();
            return true;
        }

        if (mode == "status") {
            DebugManager::printStatus();
            return true;
        }

        if (mode == "h") {
            Serial.println("DEBUG H: Zeige alle Stunden (0-23), je 1 Sekunde");
            resetClockMorphState();
            for (int hour = 0; hour < 24; hour++) {
                showTime(hour, 0);
                waitMs(1000);
            }
            Serial.println("DEBUG H: Fertig");
            return true;
        }

        if (mode == "m") {
            Serial.println("DEBUG M: Zeige alle Minuten (0-59), je 1 Sekunde");
            resetClockMorphState();
            for (int minute = 0; minute < 60; minute++) {
                showTime(5, minute);
                waitMs(1000);
            }
            Serial.println("DEBUG M: Fertig");
            return true;
        }

        if (mode == "w" || mode == "layout") {
            Serial.println("DEBUG W: Zeige alle Wortkombinationen (5-Minuten Raster), je 1 Sekunde");
            const int wordMinutes[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
            resetClockMorphState();
            for (size_t i = 0; i < (sizeof(wordMinutes) / sizeof(wordMinutes[0])); i++) {
                showTime(5, wordMinutes[i]);
                waitMs(1000);
            }
            Serial.println("DEBUG W: Fertig");
            return true;
        }

        if (mode == "s") {
            Serial.println("DEBUG S: Zeige Spezialanzeige 'Love You' fuer 1 Sekunde");
            const String previousEffect = currentEffect;
            transitionToEffect("love");
            waitMs(1000);
            transitionToEffect(previousEffect);
            Serial.println("DEBUG S: Fertig");
            return true;
        }

        int separator = mode.indexOf(' ');
        if (separator < 0) {
            Serial.println("FEHLER: Debug Help | Debug Status | Debug <category> On/Off");
            return true;
        }

        String categoryToken = mode.substring(0, separator);
        String stateToken = mode.substring(separator + 1);
        categoryToken.trim();
        stateToken.trim();

        if (categoryToken == "all") {
            bool enabled = false;
            if (!DebugManager::parseEnabled(stateToken, enabled)) {
                Serial.println("FEHLER: Debug All On|Off");
                return true;
            }
            DebugManager::setAll(enabled);
            Serial.printf("Debug all: %s\n", enabled ? "ON" : "OFF");
            return true;
        }

        DebugCategory category;
        if (!DebugManager::parseCategory(categoryToken, category)) {
            Serial.print("FEHLER: Unbekannte Debug-Kategorie: ");
            Serial.println(categoryToken);
            DebugManager::printHelp();
            return true;
        }

        bool enabled = false;
        if (!DebugManager::parseEnabled(stateToken, enabled)) {
            Serial.println("FEHLER: Status nur mit On/Off/True/False/Enable/Disable");
            return true;
        }

        DebugManager::setEnabled(category, enabled);
        Serial.printf("Debug %s: %s\n", DebugManager::categoryName(category), enabled ? "ON" : "OFF");
        return true;
    }

    static bool cmdSettime(const String& args) {
        Serial.println("Empfange neue Uhrzeit...");
        int hour = -1;
        int minute = -1;
        if (!parseHourMinuteInput(args, hour, minute)) {
            Serial.println("FEHLER: Ungueltige Uhrzeit!");
            Serial.println("Format: settime 14 30 oder settime 14:30");
            return true;
        }

        Serial.printf("Zeige Zeit: %02d:%02d\n", hour, minute);
        showTime(hour, minute);
        return true;
    }

    static bool cmdTestMorph(const String& args) {
        Serial.println("Teste Morph-Uebergang...");

        int hour = -1;
        int minute = -1;
        if (!parseHourMinuteInput(args, hour, minute)) {
            Serial.println("FEHLER: test morph HH:MM (z.B. test morph 14:35)");
            return true;
        }

        resetClockMorphState();

        Serial.printf("Start-Zeit: %02d:%02d\n", hour, minute);
        showTime(hour, minute);
        waitMs(100);

        int nextMinute = minute + 5;
        int nextHour = hour;
        if (nextMinute >= 60) {
            nextMinute = 0;
            nextHour = (hour + 1) % 24;
        }

        Serial.printf("Ziel-Zeit: %02d:%02d (Morphing-Animation ~4.8s)\n", nextHour, nextMinute);
        showTime(nextHour, nextMinute);
        Serial.println("Morph-Test abgeschlossen.");
        return true;
    }

    static bool cmdSetwifi(const String& args) {
        Serial.println("Empfange neue WLAN + MQTT Daten...");

        std::vector<String> tokens = parseQuotedTokens(String("setwifi ") + args);
        if (tokens.size() != 7) {
            Serial.println("FEHLER: Format:");
            Serial.println("setwifi SSID PASS MQTT_SERVER MQTT_USER MQTT_PASS MQTT_PORT");
            return true;
        }

        String ssid = tokens[1];
        String pass = tokens[2];
        String mqtt_s = tokens[3];
        String mqtt_u = tokens[4];
        String mqtt_p = tokens[5];
        int mqtt_prt = tokens[6].toInt();

        wifiManager.saveConfig(ssid, pass, mqtt_s, mqtt_u, mqtt_p, mqtt_prt);
        Serial.println("Gespeichert. Neustart in 1 Sekunde...");
        rebootDevice("Serial setwifi", 1000);
        return true;
    }

    static bool cmdEffect(const String& args) {
        String name = args;
        name.trim();
        if (name.isEmpty()) {
            Serial.println("FEHLER: effect <name>  (z.B. effect clock)");
            return true;
        }
        if (name != "clock" && effectManager.getEffect(name) == nullptr) {
            Serial.print("FEHLER: Unbekannter Effekt: ");
            Serial.println(name);
            return true;
        }

        applyControlUpdate(
            false, powerState,
            false, brightness,
            false, color,
            true, name,
            false,
            true,
            true
        );
        Serial.println("Effekt gesetzt: " + name);
        return true;
    }

    static bool cmdBrightness(const String& args) {
        int val = args.toInt();
        if (val < ControlConfig::BRIGHTNESS_MIN || val > ControlConfig::BRIGHTNESS_MAX) {
            Serial.println("FEHLER: brightness <0-255>");
            return true;
        }

        applyControlUpdate(
            false, powerState,
            true, (uint8_t)val,
            false, color,
            false, "",
            false,
            true,
            true
        );
        Serial.printf("Helligkeit gesetzt: %d\n", brightness);
        return true;
    }

    static bool cmdSpeed(const String& args) {
        int val = args.toInt();
        if (val < ControlConfig::SPEED_MIN || val > ControlConfig::SPEED_MAX) {
            Serial.println("FEHLER: speed <1-100>");
            return true;
        }

        effectSpeed = (uint8_t)val;
        stateManager.setSpeed(effectSpeed);
        stateManager.scheduleSave();
        Serial.printf("Geschwindigkeit gesetzt: %d\n", effectSpeed);
        return true;
    }

    static bool cmdIntensity(const String& args) {
        int val = args.toInt();
        if (val < ControlConfig::INTENSITY_MIN || val > ControlConfig::INTENSITY_MAX) {
            Serial.println("FEHLER: intensity <1-100>");
            return true;
        }

        effectIntensity = (uint8_t)val;
        stateManager.setIntensity(effectIntensity);
        stateManager.scheduleSave();
        Serial.printf("Intensitaet gesetzt: %d\n", effectIntensity);
        return true;
    }

    static bool cmdTransition(const String& args) {
        int val = args.toInt();
        if (val < ControlConfig::TRANSITION_MIN_MS || val > ControlConfig::TRANSITION_MAX_MS) {
            Serial.println("FEHLER: transition <200-10000>");
            return true;
        }

        transitionMs = (uint16_t)val;
        stateManager.setTransitionMs(transitionMs);
        stateManager.scheduleSave();
        Serial.printf("Uebergangsdauer gesetzt: %d ms\n", transitionMs);
        return true;
    }

    static bool cmdColor(const String& args) {
        int r = -1;
        int g = -1;
        int b = -1;
        if (!parseRgbInput(args, r, g, b)) {
            Serial.println("FEHLER: color <r> <g> <b>  (je 0-255)");
            return true;
        }

        uint32_t newColor = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        applyControlUpdate(
            false, powerState,
            false, brightness,
            true, newColor,
            false, "",
            false,
            true,
            true
        );
        Serial.printf("Farbe gesetzt: R=%d G=%d B=%d (0x%06X)\n", r, g, b, color);
        return true;
    }

    static bool cmdHelp(const String&) {
        SerialManager::printHelp();
        return true;
    }

    static const CommandDef kCommands[] = {
        // New canonical commands
        {"Help", MatchMode::Exact, "Help", "Shows all Serial Commands", cmdHelp, true},
        {"Status", MatchMode::Exact, "Status", "Shows current runtime status", cmdStatus, true},
        {"Diag", MatchMode::Exact, "Diag", "Shows memory diagnostics", cmdDiag, true},
        {"RTC", MatchMode::Exact, "RTC", "Shows RTC date and time", cmdRtc, true},
        {"Reboot", MatchMode::Exact, "Reboot", "Restarts device", cmdReboot, true},
        {"Creds Show", MatchMode::Exact, "Creds Show", "Shows stored WiFi/MQTT credentials", cmdCreds, true},
        {"Creds Clear", MatchMode::Exact, "Creds Clear", "Clears WiFi/MQTT credentials and reboots", cmdCredsFlush, true},
        {"Wifi Set ", MatchMode::Prefix, "Wifi Set SSID PASS MQTT_SERVER MQTT_USER MQTT_PASS MQTT_PORT", "Sets WiFi and MQTT config", cmdSetwifi, true},
        {"Setup Start", MatchMode::Exact, "Setup Start", "Starts setup mode", cmdSetup, true},
        {"OTA Info", MatchMode::Exact, "OTA Info", "Shows current firmware version and OTA profile", cmdOtaInfo, true},
        {"OTA Check", MatchMode::Exact, "OTA Check", "Checks GitHub for a newer firmware and installs it", cmdUpdateCheck, true},
        {"Update Check", MatchMode::Exact, "Update Check", "Checks GitHub for a newer firmware and installs it", cmdUpdateCheck, false},
        {"Time Set ", MatchMode::Prefix, "Time Set HH:MM", "Shows a specific time immediately", cmdSettime, true},
        {"Test Morph ", MatchMode::Prefix, "Test Morph HH:MM", "Tests clock morph transition", cmdTestMorph, true},
        {"Effect Set ", MatchMode::Prefix, "Effect Set <name>", "Sets active effect", cmdEffect, true},
        {"Brightness Set ", MatchMode::Prefix, "Brightness Set <0-255>", "Sets brightness", cmdBrightness, true},
        {"Color Set ", MatchMode::Prefix, "Color Set <r> <g> <b>", "Sets RGB color", cmdColor, true},
        {"Speed Set ", MatchMode::Prefix, "Speed Set <1-100>", "Sets effect speed", cmdSpeed, true},
        {"Intensity Set ", MatchMode::Prefix, "Intensity Set <1-100>", "Sets effect intensity", cmdIntensity, true},
        {"Transition Set ", MatchMode::Prefix, "Transition Set <ms>", "Sets transition duration (200-10000)", cmdTransition, true},
        {"Test Self", MatchMode::Exact, "Test Self", "Runs self regression test", cmdSelftest, true},
        {"Test Smoke", MatchMode::Exact, "Test Smoke", "Runs short smoke test", cmdSmoke, true},
        {"Debug ", MatchMode::Prefix, "Debug Help|Status|<category> On|Off", "Runtime debug categories and legacy H/M/W/S tests", cmdDebug, true},

        // Legacy aliases (hidden from help)
        {"help", MatchMode::Exact, "", "", cmdHelp, false},
        {"status", MatchMode::Exact, "", "", cmdStatus, false},
        {"diag", MatchMode::Exact, "", "", cmdDiag, false},
        {"rtc", MatchMode::Exact, "", "", cmdRtc, false},
        {"reboot", MatchMode::Exact, "", "", cmdReboot, false},
        {"creds", MatchMode::Exact, "", "", cmdCreds, false},
        {"creds flush", MatchMode::Exact, "", "", cmdCredsFlush, false},
        {"setwifi ", MatchMode::Prefix, "", "", cmdSetwifi, false},
        {"setup", MatchMode::Exact, "", "", cmdSetup, false},
        {"ota info", MatchMode::Exact, "", "", cmdOtaInfo, false},
        {"ota check", MatchMode::Exact, "", "", cmdUpdateCheck, false},
        {"ota", MatchMode::Exact, "", "", cmdOtaInfo, false},
        {"settime ", MatchMode::Prefix, "", "", cmdSettime, false},
        {"test morph ", MatchMode::Prefix, "", "", cmdTestMorph, false},
        {"effect ", MatchMode::Prefix, "", "", cmdEffect, false},
        {"brightness ", MatchMode::Prefix, "", "", cmdBrightness, false},
        {"color ", MatchMode::Prefix, "", "", cmdColor, false},
        {"speed ", MatchMode::Prefix, "", "", cmdSpeed, false},
        {"intensity ", MatchMode::Prefix, "", "", cmdIntensity, false},
        {"transition ", MatchMode::Prefix, "", "", cmdTransition, false},
        {"selftest", MatchMode::Exact, "", "", cmdSelftest, false},
        {"smoke", MatchMode::Exact, "", "", cmdSmoke, false},
        {"debug layout", MatchMode::Exact, "", "", cmdDebugLayout, false},

        // Short power-user aliases (hidden from help)
        {"fx ", MatchMode::Prefix, "", "", cmdEffect, false},
        {"br ", MatchMode::Prefix, "", "", cmdBrightness, false},
        {"col ", MatchMode::Prefix, "", "", cmdColor, false},
        {"sp ", MatchMode::Prefix, "", "", cmdSpeed, false},
        {"int ", MatchMode::Prefix, "", "", cmdIntensity, false},
        {"tr ", MatchMode::Prefix, "", "", cmdTransition, false}
    };

    static constexpr size_t kCommandCount = sizeof(kCommands) / sizeof(kCommands[0]);
}

namespace SerialManager {
    static char sSerialLineBuf[256];
    static size_t sSerialLineLen = 0;

    void printHelp() {
        Serial.println("============ SERIAL BEFEHLE ============");
        for (size_t i = 0; i < kCommandCount; i++) {
            if (!kCommands[i].showInHelp) {
                continue;
            }
            Serial.printf("%-42s - %s\n", kCommands[i].usage, kCommands[i].description);
        }
        Serial.println("========================================");
    }

    void handle() {
        while (Serial.available() > 0) {
            const int raw = Serial.read();
            if (raw < 0) {
                break;
            }
            const char ch = (char)raw;

            if (ch == '\r') {
                continue;
            }

            if (ch != '\n') {
                if (sSerialLineLen < (sizeof(sSerialLineBuf) - 1)) {
                    sSerialLineBuf[sSerialLineLen++] = ch;
                }
                continue;
            }

            sSerialLineBuf[sSerialLineLen] = '\0';
            String cmd = String(sSerialLineBuf);
            sSerialLineLen = 0;
            cmd.trim();

            if (cmd.isEmpty()) {
                continue;
            }

            DebugManager::printf(DebugCategory::Serial, "[SERIAL] command=%s\n", cmd.c_str());

            bool handled = false;
            for (size_t i = 0; i < kCommandCount; i++) {
                const CommandDef& c = kCommands[i];

                if (c.mode == MatchMode::Exact) {
                    if (equalsIgnoreCase(cmd, c.trigger)) {
                        c.handler("");
                        handled = true;
                        break;
                    }
                    continue;
                }

                if (startsWithIgnoreCase(cmd, c.trigger)) {
                    const String prefix = c.trigger;
                    String args = cmd.substring(prefix.length());
                    args.trim();
                    c.handler(args);
                    handled = true;
                    break;
                }
            }

            if (!handled) {
                Serial.print("Unbekannter Befehl: ");
                Serial.println(cmd);
                Serial.println("'help' fuer alle Befehle");
            }
        }
    }
}

void handleSerialCommands() {
    SerialManager::handle();
}
