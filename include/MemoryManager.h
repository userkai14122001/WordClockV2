#pragma once

#include <Arduino.h>

namespace MemoryManager {
    // Memory thresholds
    static constexpr uint32_t CRITICAL_THRESHOLD = 4096;      // 4 KB - disable heavy effects
    static constexpr uint32_t WARNING_THRESHOLD = 8192;       // 8 KB - warn user
    
    // Effects classified as "heavy" (large buffers or intensive calc)
    static const char* heavyEffects[] = {
        "fire2d",
        "matrix",
        "plasma"
    };
    static constexpr size_t HEAVY_EFFECTS_COUNT = 3;
    
    // Get current free RAM
    uint32_t getFreeRam();
    
    // Check if we're in critical memory state
    bool isCritical();
    
    // Check if we're in warning state
    bool isWarning();
    
    // Check if an effect is "heavy"
    bool isHeavyEffect(const String& effectName);
    
    enum class MemoryLevel {
        Ok,
        Warning,
        Critical
    };

    // Pure decision helper for runtime fallback logic.
    bool shouldFallbackToSafeEffect(bool powerOn, const String& activeEffect);

    // Classify current memory level.
    MemoryLevel getMemoryLevel();

    // Stable ASCII status text (for logs / API / serial output).
    const char* memoryLevelText(MemoryLevel level);

    // Log low-memory fallback action.
    void logFallbackAction(const String& fromEffect, const String& toEffect);
    
    // Debug output
    void printMemoryStats();
}
