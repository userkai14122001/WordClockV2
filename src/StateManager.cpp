#include "StateManager.h"
#include <Preferences.h>
#include "ControlConfig.h"
#include "DebugManager.h"

StateManager& StateManager::getInstance() {
    static StateManager instance;
    return instance;
}

void StateManager::setPowerState(bool power) {
    if (power_state != power) {
        bool old_state = power_state;
        power_state = power;
        
        DebugManager::print(DebugCategory::State, "StateManager: Power ");
        DebugManager::println(DebugCategory::State, power ? "ON" : "OFF");
        
        if (on_power_change) {
            on_power_change(old_state, power);
        }
    }
}

void StateManager::togglePower() {
    setPowerState(!power_state);
}

void StateManager::setCurrentEffect(const String& effect) {
    if (current_effect != effect) {
        String old_effect = current_effect;
        current_effect = effect;
        
        DebugManager::print(DebugCategory::State, "StateManager: Effect changed to ");
        DebugManager::println(DebugCategory::State, effect);
        
        if (on_effect_change) {
            on_effect_change(old_effect, effect);
        }
    }
}

void StateManager::setColor(uint32_t rgb) {
    if (color != rgb) {
        uint32_t old_color = color;
        color = rgb;
        
        DebugManager::print(DebugCategory::State, "StateManager: Color changed to 0x");
        DebugManager::println(DebugCategory::State, rgb, HEX);
        
        if (on_color_change) {
            on_color_change(old_color, rgb);
        }
    }
}

void StateManager::setColor(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t rgb = (r << 16) | (g << 8) | b;
    setColor(rgb);
}

void StateManager::setBrightness(uint8_t brightness) {
    uint8_t newBrightness = constrain(brightness, ControlConfig::BRIGHTNESS_MIN, ControlConfig::BRIGHTNESS_MAX);

    if (this->brightness != newBrightness) {
        uint8_t old_brightness = this->brightness;
        this->brightness = newBrightness;

        DebugManager::print(DebugCategory::State, "StateManager: Brightness changed to ");
        DebugManager::println(DebugCategory::State, newBrightness);

        if (on_brightness_change) {
            on_brightness_change(old_brightness, newBrightness);
        }
    }
}

void StateManager::increaseBrightness(uint8_t step) {
    setBrightness(brightness + step);
}

void StateManager::decreaseBrightness(uint8_t step) {
    setBrightness(brightness - step);
}

void StateManager::setSpeed(uint8_t speed) {
    speed = constrain(speed, ControlConfig::SPEED_MIN, ControlConfig::SPEED_MAX);
    this->speed = speed;
    DebugManager::print(DebugCategory::State, "StateManager: Speed set to ");
    DebugManager::println(DebugCategory::State, speed);
}

void StateManager::setIntensity(uint8_t intensity) {
    intensity = constrain(intensity, ControlConfig::INTENSITY_MIN, ControlConfig::INTENSITY_MAX);
    this->intensity = intensity;
    DebugManager::print(DebugCategory::State, "StateManager: Intensity set to ");
    DebugManager::println(DebugCategory::State, intensity);
}

void StateManager::setDensity(uint8_t density) {
    density = constrain(density, ControlConfig::DENSITY_MIN, ControlConfig::DENSITY_MAX);
    this->density = density;
    DebugManager::print(DebugCategory::State, "StateManager: Density set to ");
    DebugManager::println(DebugCategory::State, density);
}

void StateManager::setTransitionMs(uint16_t transition_ms) {
    transition_ms = constrain(transition_ms, ControlConfig::TRANSITION_MIN_MS, ControlConfig::TRANSITION_MAX_MS);
    this->transition_ms = transition_ms;
    DebugManager::print(DebugCategory::State, "StateManager: TransitionMs set to ");
    DebugManager::println(DebugCategory::State, transition_ms);
}

void StateManager::setPalette(uint8_t p) {
    p = constrain(p, ControlConfig::PALETTE_MIN, ControlConfig::PALETTE_MAX);
    this->palette = p;
    DebugManager::print(DebugCategory::State, "StateManager: Palette set to ");
    DebugManager::println(DebugCategory::State, p);
}

void StateManager::setHueShift(uint16_t hs) {
    hs = constrain(hs, ControlConfig::HUE_SHIFT_MIN, ControlConfig::HUE_SHIFT_MAX);
    this->hue_shift = hs;
    DebugManager::print(DebugCategory::State, "StateManager: HueShift set to ");
    DebugManager::println(DebugCategory::State, hs);
}

void StateManager::onPowerStateChange(PowerStateChangeCallback cb) {
    on_power_change = cb;
}

void StateManager::onColorChange(ColorChangeCallback cb) {
    on_color_change = cb;
}

void StateManager::onBrightnessChange(BrightnessChangeCallback cb) {
    on_brightness_change = cb;
}

void StateManager::onEffectChange(EffectChangeCallback cb) {
    on_effect_change = cb;
}

void StateManager::saveToPreferences() {
    save_pending = false;
    Preferences prefs;
    if (!prefs.begin("state", false)) {
        DebugManager::println(DebugCategory::State, "StateManager: Failed to save preferences");
        return;
    }
    
    prefs.putBool("power", power_state);
    prefs.putString("effect", current_effect);
    prefs.putUInt("color", color);
    prefs.putUChar("brightness", brightness);
    prefs.putUChar("speed", speed);
    prefs.putUChar("intensity", intensity);
    prefs.putUChar("density", density);
    prefs.putUShort("transition", transition_ms);
    prefs.putUChar("palette", palette);
    prefs.putUShort("hue_shift", hue_shift);
    
    prefs.end();
    DebugManager::println(DebugCategory::State, "StateManager: State saved to preferences");
}

void StateManager::scheduleSave(uint32_t delay_ms) {
    save_pending = true;
    save_deadline_ms = millis() + delay_ms;
}

void StateManager::processPendingSave() {
    if (!save_pending) {
        return;
    }

    if ((long)(millis() - save_deadline_ms) < 0) {
        return;
    }

    saveToPreferences();
}

void StateManager::loadFromPreferences() {
    Preferences prefs;
    if (!prefs.begin("state", true)) {
        DebugManager::println(DebugCategory::State, "StateManager: Failed to load preferences");
        return;
    }
    
    power_state = prefs.getBool("power", true);
    current_effect = prefs.getString("effect", "clock");
    color = prefs.getUInt("color", ControlConfig::DEFAULT_COLOR);
    brightness = prefs.getUChar("brightness", ControlConfig::DEFAULT_BRIGHTNESS);
    speed = prefs.getUChar("speed", ControlConfig::DEFAULT_SPEED);
    intensity = prefs.getUChar("intensity", ControlConfig::DEFAULT_INTENSITY);
    density = prefs.getUChar("density", ControlConfig::DEFAULT_DENSITY);
    transition_ms = prefs.getUShort("transition", ControlConfig::DEFAULT_TRANSITION_MS);
    palette   = prefs.getUChar("palette", ControlConfig::DEFAULT_PALETTE);
    hue_shift = prefs.getUShort("hue_shift", ControlConfig::DEFAULT_HUE_SHIFT);
    
    prefs.end();
    DebugManager::println(DebugCategory::State, "StateManager: State loaded from preferences");
}

void StateManager::printState() const {
    Serial.println("=== StateManager ===");
    Serial.print("Power: ");
    Serial.println(power_state ? "ON" : "OFF");
    Serial.print("Effect: ");
    Serial.println(current_effect);
    Serial.print("Color: 0x");
    Serial.println(color, HEX);
    Serial.print("Brightness: ");
    Serial.println(brightness);
    Serial.print("Speed: ");
    Serial.println(speed);
    Serial.print("Intensity: ");
    Serial.println(intensity);
    Serial.print("TransitionMs: ");
    Serial.println(transition_ms);
}
