#pragma once

#include <Arduino.h>

enum class DebugCategory : uint8_t {
    Boot,
    Main,
    Loop,
    MQTT,
    WiFi,
    RTC,
    Time,
    State,
    EffectManager,
    Effects,
    LEDMatrix,
    Memory,
    OTA,
    Serial,
    Test,
    Count
};

namespace DebugManager {
    bool isEnabled(DebugCategory category);
    void setEnabled(DebugCategory category, bool enabled);
    void setAll(bool enabled);

    const char* categoryName(DebugCategory category);
    const char* categoryDescription(DebugCategory category);
    size_t categoryCount();

    bool parseCategory(const String& input, DebugCategory& category);
    bool parseEnabled(const String& input, bool& enabled);

    void printHelp();
    void printStatus();

    template <typename T>
    inline void print(DebugCategory category, const T& value) {
        if (isEnabled(category)) {
            Serial.print(value);
        }
    }

    inline void println(DebugCategory category) {
        if (isEnabled(category)) {
            Serial.println();
        }
    }

    template <typename T>
    inline void println(DebugCategory category, const T& value) {
        if (isEnabled(category)) {
            Serial.println(value);
        }
    }

    template <typename T>
    inline void println(DebugCategory category, const T& value, int format) {
        if (isEnabled(category)) {
            Serial.println(value, format);
        }
    }

    void printf(DebugCategory category, const char* format, ...);
}
