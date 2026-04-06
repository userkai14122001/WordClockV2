#include "MemoryManager.h"
#include "DebugManager.h"

namespace MemoryManager {
    
    uint32_t getFreeRam() {
        return ESP.getFreeHeap();
    }
    
    bool isCritical() {
        return getFreeRam() < CRITICAL_THRESHOLD;
    }
    
    bool isWarning() {
        return getFreeRam() < WARNING_THRESHOLD;
    }

    MemoryLevel getMemoryLevel() {
        if (isCritical()) return MemoryLevel::Critical;
        if (isWarning()) return MemoryLevel::Warning;
        return MemoryLevel::Ok;
    }

    const char* memoryLevelText(MemoryLevel level) {
        switch (level) {
            case MemoryLevel::Critical: return "CRITICAL";
            case MemoryLevel::Warning:  return "WARNING";
            case MemoryLevel::Ok:
            default:                    return "OK";
        }
    }
    
    bool isHeavyEffect(const String& effectName) {
        for (size_t i = 0; i < HEAVY_EFFECTS_COUNT; i++) {
            if (effectName == heavyEffects[i]) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldFallbackToSafeEffect(bool powerOn, const String& activeEffect) {
        if (!powerOn) {
            return false;
        }
        if (!isCritical()) {
            return false;
        }
        return isHeavyEffect(activeEffect);
    }

    void logFallbackAction(const String& fromEffect, const String& toEffect) {
        DebugManager::println(DebugCategory::Memory, "WARN: low memory - switching to safe effect");
        DebugManager::printf(DebugCategory::Memory, "Free RAM: %u bytes (critical threshold: %u)\n", getFreeRam(), CRITICAL_THRESHOLD);
        DebugManager::print(DebugCategory::Memory, "Effect switch: '");
        DebugManager::print(DebugCategory::Memory, fromEffect);
        DebugManager::print(DebugCategory::Memory, "' -> '");
        DebugManager::print(DebugCategory::Memory, toEffect);
        DebugManager::println(DebugCategory::Memory, "'");
    }
    
    void printMemoryStats() {
        uint32_t free = getFreeRam();
        uint32_t total = ESP.getHeapSize();
        float percent = 100.0f * (total - free) / total;
        
        Serial.println();
        Serial.println("=== MEMORY STATUS ===");
        Serial.printf("Free RAM:     %u bytes\n", free);
        Serial.printf("Total Heap:   %u bytes\n", total);
        Serial.printf("Used:         %.1f%%\n", percent);
        Serial.printf("Max Alloc:    %u bytes\n", ESP.getMaxAllocHeap());
        Serial.printf("Min Ever:     %u bytes\n", ESP.getMinFreeHeap());
        
        const MemoryLevel level = getMemoryLevel();
        Serial.print("STATUS: ");
        Serial.println(memoryLevelText(level));
        Serial.println("====================");
    }
}
