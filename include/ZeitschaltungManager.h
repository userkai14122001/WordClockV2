#pragma once

#include <Arduino.h>
#include <vector>

// Zeitschaltung rule definition
struct ZeitschaltungRule {
    bool enabled;           // Rule active/inactive
    String name;            // Rule name (default: "1" to "5")
    uint8_t hour;           // Trigger hour (0-23)
    uint8_t minute;         // Trigger minute (0-59)
    bool actionPower;       // Action: power on/off
    uint8_t actionBrightness;  // Action: brightness percent (1-100)
    uint8_t actionSpeed;       // Action: speed percent (1-100)
    uint8_t actionIntensity;   // Action: intensity percent (1-100)
    uint8_t actionDensity;     // Action: density percent (1-100)
    uint16_t actionTransition; // Action: transition in ms (200-10000)
    String actionEffect;    // Action: effect name, empty = no change
    
    ZeitschaltungRule() 
                : enabled(false), name(""), hour(0), minute(0), actionPower(true),
                      actionBrightness(50), actionSpeed(50), actionIntensity(50),
                      actionDensity(50), actionTransition(1000), actionEffect("") {}
    
    ZeitschaltungRule(const String& defaultName) 
                : enabled(false), name(defaultName), hour(0), minute(0), actionPower(true),
                      actionBrightness(50), actionSpeed(50), actionIntensity(50),
                      actionDensity(50), actionTransition(1000), actionEffect("") {}
};

class ZeitschaltungManager {
public:
    static ZeitschaltungManager& getInstance();
    
    // Rules management
    void init();
    
    // Get rules
    const std::vector<ZeitschaltungRule>& getRules() const { return rules; }
    ZeitschaltungRule& getRule(size_t index) {
        if (index >= rules.size()) index = rules.size() - 1;
        return rules[index];
    }
    
    // Set/update rule
    void setRule(size_t index, const ZeitschaltungRule& rule);
    
    // Clear all rules
    void clearAllRules();
    
    // Check and apply active rule based on current time
    // Returns true if a rule was triggered
    bool checkAndApplyRule(uint8_t hour, uint8_t minute);
    
    // Get current active rule index (-1 if none)
    int getActiveRuleIndex() const { return lastTriggeredRuleIndex; }
    
    // Save to persistent storage (immediate)
    void save();

    // Schedule a deferred save (debounced) – safe to call from MQTT callbacks
    void scheduleSave(uint32_t delay_ms = 500);

    // Must be called from the main loop to flush a pending deferred save
    void processPendingSave();

    // Load from persistent storage
    void load();
    
    // Debug output
    void printRules();
    
private:
    ZeitschaltungManager();
    static ZeitschaltungManager* instance;
    
    static constexpr size_t MAX_RULES = 5;
    std::vector<ZeitschaltungRule> rules;
    int lastTriggeredRuleIndex = -1;
    unsigned long lastCheckMs = 0;
    static constexpr unsigned long CHECK_INTERVAL_MS = 1000;

    bool save_pending = false;
    unsigned long save_deadline_ms = 0;
};
