#ifndef EFFECTMANAGER_H
#define EFFECTMANAGER_H

#include <Arduino.h>
#include "effects.h"
#include <map>
#include <vector>

class EffectManager {
public:
    enum class TransitionState {
        Idle,
        FadingOut,
        Switching,
        FadingIn
    };

    // Singleton
    static EffectManager& getInstance();
    
    // Effect registration
    void registerEffect(const String& name, Effect* effect);
    Effect* getEffect(const String& name) const;
    
    // Effect lifecycle
    void setEffect(const String& name);
    String getCurrentEffectName() const { return current_effect_name; }
    Effect* getCurrentEffect() const { return current_effect; }
    
    // Update loop - call from main loop
    void update();
    
    // List all registered effects
    void listEffects() const;
    size_t getEffectCount() const { return effects.size(); }
    
    // Transitions (optional)
    void transitionToEffect(const String& name, uint16_t duration_ms = 0);
    
    // Speed control for effects
    void setEffectSpeed(uint8_t speed);
    
    // Reset current effect
    void resetCurrentEffect();
    
private:
    EffectManager() = default;
    
    // Registered effects
    std::map<String, Effect*> effects;
    
    // Current state
    String current_effect_name = "";
    Effect* current_effect = nullptr;
    uint8_t effect_speed = 50;
    
    // Transition state
    bool is_transitioning = false;
    unsigned long transition_start = 0;
    uint16_t transition_duration = 0;
    uint16_t transition_half_duration = 0;
    String next_effect_name = "";
    TransitionState transition_state = TransitionState::Idle;
};

#endif
