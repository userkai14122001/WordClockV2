#include "DebugManager.h"

#include <stdarg.h>
#include <string.h>

namespace {
    struct DebugCategoryMeta {
        DebugCategory category;
        const char* name;
        const char* description;
        const char* alias1;
        const char* alias2;
        const char* alias3;
        bool defaultEnabled;
    };

    DebugCategoryMeta gDebugCategories[] = {
        {DebugCategory::Boot, "boot", "Boot sequence and startup flow", "startup", nullptr, nullptr, false},
        {DebugCategory::Main, "main", "Main control flow and control updates", "core", nullptr, nullptr, false},
        {DebugCategory::Loop, "loop", "Main loop timing warnings", "timing", nullptr, nullptr, false},
        {DebugCategory::MQTT, "mqtt", "MQTT connect, callback and publish logs", "broker", nullptr, nullptr, false},
        {DebugCategory::WiFi, "wifi", "WiFi, web server and setup mode logs", "web", "network", nullptr, false},
        {DebugCategory::RTC, "rtc", "RTC and I2C diagnostics", "clockchip", nullptr, nullptr, false},
        {DebugCategory::Time, "time", "NTP and timezone sync logs", "ntp", nullptr, nullptr, false},
        {DebugCategory::State, "state", "State persistence and state changes", "prefs", "preferences", nullptr, false},
        {DebugCategory::EffectManager, "effectmanager", "Effect manager switching and registration", "fxmgr", "effects-manager", nullptr, false},
        {DebugCategory::Effects, "effects", "Individual effect diagnostics", "effect", "fx", nullptr, false},
        {DebugCategory::LEDMatrix, "ledmatrix", "LED matrix initialization and direct LED diagnostics", "led", "matrix", nullptr, false},
        {DebugCategory::Memory, "memory", "Heap and fallback diagnostics", "ram", nullptr, nullptr, false},
        {DebugCategory::OTA, "ota", "OTA update diagnostics", "update", nullptr, nullptr, false},
        {DebugCategory::Serial, "serial", "Serial command parser diagnostics", "cli", "console", nullptr, false},
        {DebugCategory::Test, "test", "Smoke, selftest and manual debug routines", "tests", nullptr, nullptr, false},
    };

    bool gDebugEnabled[static_cast<size_t>(DebugCategory::Count)] = {
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
    };

    bool equalsIgnoreCase(const String& input, const char* candidate) {
        String left = input;
        String right(candidate);
        left.trim();
        right.trim();
        left.toLowerCase();
        right.toLowerCase();
        return left == right;
    }
}

namespace DebugManager {
    bool isEnabled(DebugCategory category) {
        const size_t index = static_cast<size_t>(category);
        if (index >= static_cast<size_t>(DebugCategory::Count)) {
            return false;
        }
        return gDebugEnabled[index];
    }

    void setEnabled(DebugCategory category, bool enabled) {
        const size_t index = static_cast<size_t>(category);
        if (index >= static_cast<size_t>(DebugCategory::Count)) {
            return;
        }
        gDebugEnabled[index] = enabled;
    }

    void setAll(bool enabled) {
        for (size_t i = 0; i < static_cast<size_t>(DebugCategory::Count); i++) {
            gDebugEnabled[i] = enabled;
        }
    }

    const char* categoryName(DebugCategory category) {
        for (const auto& meta : gDebugCategories) {
            if (meta.category == category) {
                return meta.name;
            }
        }
        return "unknown";
    }

    const char* categoryDescription(DebugCategory category) {
        for (const auto& meta : gDebugCategories) {
            if (meta.category == category) {
                return meta.description;
            }
        }
        return "";
    }

    size_t categoryCount() {
        return sizeof(gDebugCategories) / sizeof(gDebugCategories[0]);
    }

    bool parseCategory(const String& input, DebugCategory& category) {
        for (const auto& meta : gDebugCategories) {
            if (equalsIgnoreCase(input, meta.name) ||
                (meta.alias1 != nullptr && equalsIgnoreCase(input, meta.alias1)) ||
                (meta.alias2 != nullptr && equalsIgnoreCase(input, meta.alias2)) ||
                (meta.alias3 != nullptr && equalsIgnoreCase(input, meta.alias3))) {
                category = meta.category;
                return true;
            }
        }
        return false;
    }

    bool parseEnabled(const String& input, bool& enabled) {
        if (equalsIgnoreCase(input, "1") ||
            equalsIgnoreCase(input, "true") ||
            equalsIgnoreCase(input, "on") ||
            equalsIgnoreCase(input, "enable") ||
            equalsIgnoreCase(input, "enabled")) {
            enabled = true;
            return true;
        }

        if (equalsIgnoreCase(input, "0") ||
            equalsIgnoreCase(input, "false") ||
            equalsIgnoreCase(input, "off") ||
            equalsIgnoreCase(input, "of") ||
            equalsIgnoreCase(input, "disable") ||
            equalsIgnoreCase(input, "disabled")) {
            enabled = false;
            return true;
        }

        return false;
    }

    void printHelp() {
        Serial.println("============ DEBUG COMMANDS ============");
        Serial.println("Debug Help");
        Serial.println("Debug Status");
        Serial.println("Debug All On|Off");
        Serial.println("Debug <category> On|Off|True|False|Enable|Disable");
        Serial.println("Verfuegbare Kategorien:");
        for (const auto& meta : gDebugCategories) {
            Serial.printf("  %-14s - %s\n", meta.name, meta.description);
        }
        Serial.println("Legacy clock tests bleiben verfuegbar: Debug H|M|W|S");
        Serial.println("========================================");
    }

    void printStatus() {
        Serial.println("============ DEBUG STATUS ==============");
        for (const auto& meta : gDebugCategories) {
            Serial.printf("%-14s : %s\n", meta.name, isEnabled(meta.category) ? "ON" : "OFF");
        }
        Serial.println("========================================");
    }

    void printf(DebugCategory category, const char* format, ...) {
        if (!isEnabled(category)) {
            return;
        }

        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.print(buffer);
    }
}