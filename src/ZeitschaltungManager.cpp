#include "ZeitschaltungManager.h"
#include "DebugManager.h"
#include "StateManager.h"
#include "ControlConfig.h"
#include "MQTTManager.h"
#include <Preferences.h>

ZeitschaltungManager* ZeitschaltungManager::instance = nullptr;

ZeitschaltungManager::ZeitschaltungManager() {
    for (size_t i = 0; i < MAX_RULES; i++) {
        rules.push_back(ZeitschaltungRule(String(i + 1)));
    }
}

ZeitschaltungManager& ZeitschaltungManager::getInstance() {
    if (!instance) {
        instance = new ZeitschaltungManager();
    }
    return *instance;
}

void ZeitschaltungManager::init() {
    DebugManager::println(DebugCategory::Main, "ZeitschaltungManager: Initializing...");
    load();
    printRules();
}

void ZeitschaltungManager::setRule(size_t index, const ZeitschaltungRule& rule) {
    if (index >= MAX_RULES) return;
    
    rules[index] = rule;
    save();
    
    DebugManager::printf(DebugCategory::Main, 
        "ZeitschaltungManager: Rule %u (\"%.20s\") updated - enabled=%d, time=%02d:%02d, power=%d, b=%u s=%u i=%u d=%u t=%ums, effect=%s\n",
        (unsigned int)index, rule.name.c_str(), rule.enabled, rule.hour, rule.minute, rule.actionPower, 
        rule.actionBrightness, rule.actionSpeed, rule.actionIntensity, rule.actionDensity,
        rule.actionTransition, rule.actionEffect.c_str());
}

void ZeitschaltungManager::clearAllRules() {
    for (size_t i = 0; i < MAX_RULES; i++) {
        rules[i] = ZeitschaltungRule(String(i + 1));
    }
    lastTriggeredRuleIndex = -1;
    save();
    DebugManager::println(DebugCategory::Main, "ZeitschaltungManager: All rules cleared");
}

bool ZeitschaltungManager::checkAndApplyRule(uint8_t hour, uint8_t minute) {
    unsigned long now = millis();
    if (now - lastCheckMs < CHECK_INTERVAL_MS) {
        return false;
    }
    lastCheckMs = now;
    
    int triggeredIndex = -1;
    
    // Check all rules in order, take the last (highest-priority rule)
    for (size_t i = 0; i < MAX_RULES; i++) {
        if (rules[i].enabled && rules[i].hour == hour && rules[i].minute == minute) {
            triggeredIndex = i;
        }
    }
    
    // If rule triggered and different from last triggered, apply it
    if (triggeredIndex != lastTriggeredRuleIndex && triggeredIndex >= 0) {
        lastTriggeredRuleIndex = triggeredIndex;
        const ZeitschaltungRule& rule = rules[triggeredIndex];
        
        DebugManager::printf(DebugCategory::Main,
            "ZeitschaltungManager: Rule %d triggered at %02d:%02d, applying action\n",
            triggeredIndex, hour, minute);
        
        // Apply action: power state
        extern void applyControlUpdate(
            bool hasPower, bool newPower,
            bool hasBrightness, uint8_t newBrightness,
            bool hasColor, uint32_t newColor,
            bool hasEffect, const String& newEffect,
            bool debounceColor,
            bool renderNow,
            bool publishState
        );
        extern bool powerState;
        extern uint8_t brightness;
        extern uint32_t color;
        extern String currentEffect;
        extern uint8_t effectSpeed;
        extern uint8_t effectIntensity;
        extern uint8_t effectDensity;
        extern uint16_t transitionMs;
        extern MQTTManager mqttManager;
        StateManager& stateManager = StateManager::getInstance();
        
        // Build action flags
        bool hasAction = false;
        bool applyPower = false;
        uint8_t applyBrightness = brightness;
        String applyEffect = currentEffect;
        
        // Power action always applies
        applyPower = rule.actionPower;
        hasAction = true;
        
        bool hasBrightness = false;
        if (rule.actionPower) {
            const uint8_t clampedBrightness = (uint8_t)constrain((int)rule.actionBrightness, 1, 100);
            applyBrightness = (uint8_t)((clampedBrightness * 255U) / 100U);
            hasBrightness = true;
            hasAction = true;
        }
        
        // Effect action (empty = no change)
        bool hasEffect = false;
        if (rule.actionPower && !rule.actionEffect.isEmpty() && rule.actionEffect != "") {
            applyEffect = rule.actionEffect;
            hasEffect = true;
            hasAction = true;
        }

        if (rule.actionPower) {
            effectSpeed = (uint8_t)constrain((int)rule.actionSpeed, 1, 100);
            effectIntensity = (uint8_t)constrain((int)rule.actionIntensity, 1, 100);
            effectDensity = (uint8_t)constrain((int)rule.actionDensity, 1, 100);
            transitionMs = (uint16_t)constrain((int)rule.actionTransition,
                                               (int)ControlConfig::TRANSITION_MIN_MS,
                                               (int)ControlConfig::TRANSITION_MAX_MS);

            stateManager.setSpeed(effectSpeed);
            stateManager.setIntensity(effectIntensity);
            stateManager.setDensity(effectDensity);
            stateManager.setTransitionMs(transitionMs);
            stateManager.scheduleSave();

            if (mqttManager.isConnected()) {
                mqttManager.publishTuningState();
            }
        }
        
        if (hasAction) {
            applyControlUpdate(
                true, applyPower,
                hasBrightness, applyBrightness,
                false, color,
                hasEffect, applyEffect,
                false,
                true,
                true
            );
        }
        
        return true;
    }
    
    return false;
}

void ZeitschaltungManager::save() {
    Preferences prefs;
    prefs.begin("wordclock", false);
    
    for (size_t i = 0; i < MAX_RULES; i++) {
        String prefix = "zs_" + String(i) + "_";
        
        prefs.putBool((prefix + "en").c_str(), rules[i].enabled);
        prefs.putString((prefix + "nm").c_str(), rules[i].name);
        prefs.putUChar((prefix + "h").c_str(), rules[i].hour);
        prefs.putUChar((prefix + "m").c_str(), rules[i].minute);
        prefs.putBool((prefix + "ap").c_str(), rules[i].actionPower);
        prefs.putUChar((prefix + "ab").c_str(), rules[i].actionBrightness);
        prefs.putUChar((prefix + "as").c_str(), rules[i].actionSpeed);
        prefs.putUChar((prefix + "ai").c_str(), rules[i].actionIntensity);
        prefs.putUChar((prefix + "ad").c_str(), rules[i].actionDensity);
        prefs.putUShort((prefix + "at").c_str(), rules[i].actionTransition);
        prefs.putString((prefix + "ae").c_str(), rules[i].actionEffect);
    }
    
    prefs.end();
    DebugManager::println(DebugCategory::Main, "ZeitschaltungManager: Rules saved");
}

void ZeitschaltungManager::load() {
    Preferences prefs;
    prefs.begin("wordclock", true);
    
    for (size_t i = 0; i < MAX_RULES; i++) {
        String prefix = "zs_" + String(i) + "_";
        
        rules[i].enabled = prefs.getBool((prefix + "en").c_str(), false);
        rules[i].name = prefs.getString((prefix + "nm").c_str(), String(i + 1));
        rules[i].hour = prefs.getUChar((prefix + "h").c_str(), 0);
        rules[i].minute = prefs.getUChar((prefix + "m").c_str(), 0);
        rules[i].actionPower = prefs.getBool((prefix + "ap").c_str(), true);
        rules[i].actionBrightness = (uint8_t)constrain((int)prefs.getUChar((prefix + "ab").c_str(), 50), 1, 100);
        rules[i].actionSpeed = (uint8_t)constrain((int)prefs.getUChar((prefix + "as").c_str(), 50), 1, 100);
        rules[i].actionIntensity = (uint8_t)constrain((int)prefs.getUChar((prefix + "ai").c_str(), 50), 1, 100);
        rules[i].actionDensity = (uint8_t)constrain((int)prefs.getUChar((prefix + "ad").c_str(), 50), 1, 100);
        uint16_t transitionMsStored = prefs.getUShort((prefix + "at").c_str(), 0);
        if (transitionMsStored == 0) {
            // Backward compatibility: old firmware stored 1..100 as percent.
            const uint8_t legacyPercent = prefs.getUChar((prefix + "at").c_str(), 50);
            const uint8_t p = (uint8_t)constrain((int)legacyPercent, 1, 100);
            transitionMsStored = (uint16_t)(ControlConfig::TRANSITION_MIN_MS +
                ((uint32_t)(p - 1U) * (uint32_t)(ControlConfig::TRANSITION_MAX_MS - ControlConfig::TRANSITION_MIN_MS)) / 99U);
        }
        rules[i].actionTransition = (uint16_t)constrain((int)transitionMsStored,
            (int)ControlConfig::TRANSITION_MIN_MS,
            (int)ControlConfig::TRANSITION_MAX_MS);
        rules[i].actionEffect = prefs.getString((prefix + "ae").c_str(), "");
    }
    
    prefs.end();
    DebugManager::println(DebugCategory::Main, "ZeitschaltungManager: Rules loaded");
}

void ZeitschaltungManager::printRules() {
    DebugManager::println(DebugCategory::Main, "========== ZEITSCHALTUNG RULES ==========");
    for (size_t i = 0; i < MAX_RULES; i++) {
        DebugManager::printf(DebugCategory::Main,
            "[%u] %s (ena=%d): %02d:%02d -> Power=%s, B=%u S=%u I=%u D=%u T=%ums, Effect=%s\n",
            (unsigned int)i, rules[i].name.c_str(), rules[i].enabled,
            rules[i].hour, rules[i].minute,
            rules[i].actionPower ? "ON" : "OFF",
            rules[i].actionBrightness,
            rules[i].actionSpeed,
            rules[i].actionIntensity,
            rules[i].actionDensity,
            rules[i].actionTransition,
            rules[i].actionEffect.isEmpty() ? "(no change)" : rules[i].actionEffect.c_str());
    }
    DebugManager::println(DebugCategory::Main, "=====================================");
}
