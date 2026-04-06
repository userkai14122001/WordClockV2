#include "EffectManager.h"
#include "DebugManager.h"

EffectManager& EffectManager::getInstance() {
    static EffectManager instance;
    return instance;
}

void EffectManager::registerEffect(const String& name, Effect* effect) {
    if (effect == nullptr) {
        DebugManager::print(DebugCategory::EffectManager, "EffectManager: Cannot register NULL effect ");
        DebugManager::println(DebugCategory::EffectManager, name);
        return;
    }
    
    effects[name] = effect;
    DebugManager::print(DebugCategory::EffectManager, "EffectManager: Registered effect '");
    DebugManager::print(DebugCategory::EffectManager, name);
    DebugManager::println(DebugCategory::EffectManager, "'");
}

Effect* EffectManager::getEffect(const String& name) const {
    auto it = effects.find(name);
    if (it != effects.end()) {
        return it->second;
    }
    return nullptr;
}

void EffectManager::setEffect(const String& name) {
    Effect* effect = getEffect(name);
    
    if (effect == nullptr) {
        DebugManager::print(DebugCategory::EffectManager, "EffectManager: Effect '");
        DebugManager::print(DebugCategory::EffectManager, name);
        DebugManager::println(DebugCategory::EffectManager, "' not found");
        return;
    }
    
    // Reset old effect if exists
    if (current_effect != nullptr) {
        current_effect->reset();
    }
    
    current_effect_name = name;
    current_effect = effect;
    
    DebugManager::print(DebugCategory::EffectManager, "EffectManager: Switched to effect '");
    DebugManager::print(DebugCategory::EffectManager, name);
    DebugManager::println(DebugCategory::EffectManager, "'");
}

void EffectManager::update() {
    if (is_transitioning) {
        const unsigned long now = millis();
        const unsigned long elapsed = now - transition_start;

        switch (transition_state) {
            case TransitionState::FadingOut:
                if (elapsed >= transition_half_duration) {
                    transition_state = TransitionState::Switching;
                }
                break;

            case TransitionState::Switching:
                setEffect(next_effect_name);
                transition_start = now;
                transition_state = TransitionState::FadingIn;
                break;

            case TransitionState::FadingIn:
                if (elapsed >= transition_half_duration) {
                    is_transitioning = false;
                    transition_state = TransitionState::Idle;
                    next_effect_name = "";
                }
                break;

            case TransitionState::Idle:
            default:
                is_transitioning = false;
                next_effect_name = "";
                break;
        }
    }

    if (current_effect != nullptr) {
        current_effect->update();
    }
}

void EffectManager::listEffects() const {
    DebugManager::println(DebugCategory::EffectManager, "=== Registered Effects ===");
    for (const auto& pair : effects) {
        DebugManager::print(DebugCategory::EffectManager, "  - ");
        DebugManager::println(DebugCategory::EffectManager, pair.first);
    }
    if (effects.empty()) {
        DebugManager::println(DebugCategory::EffectManager, "  (none)");
    }
}

void EffectManager::transitionToEffect(const String& name, uint16_t duration_ms) {
    if (getEffect(name) == nullptr) {
        DebugManager::println(DebugCategory::EffectManager, "EffectManager: Target effect not found");
        return;
    }

    if (name == current_effect_name) {
        return;
    }
    
    if (duration_ms == 0) {
        // Instant switch
        setEffect(name);
        return;
    }
    
    // Start transition
    is_transitioning = true;
    transition_start = millis();
    transition_duration = duration_ms;
    transition_half_duration = (duration_ms / 2);
    if (transition_half_duration == 0) {
        transition_half_duration = 1;
    }
    next_effect_name = name;
    transition_state = TransitionState::FadingOut;
    
    DebugManager::print(DebugCategory::EffectManager, "EffectManager: Transitioning to '");
    DebugManager::print(DebugCategory::EffectManager, name);
    DebugManager::print(DebugCategory::EffectManager, "' over ");
    DebugManager::print(DebugCategory::EffectManager, duration_ms);
    DebugManager::println(DebugCategory::EffectManager, "ms");
}

void EffectManager::setEffectSpeed(uint8_t speed) {
    effect_speed = constrain(speed, 0, 100);
    DebugManager::print(DebugCategory::EffectManager, "EffectManager: Effect speed set to ");
    DebugManager::println(DebugCategory::EffectManager, effect_speed);
}

void EffectManager::resetCurrentEffect() {
    if (current_effect != nullptr) {
        current_effect->reset();
        DebugManager::println(DebugCategory::EffectManager, "EffectManager: Current effect reset");
    }
}
