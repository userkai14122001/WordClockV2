#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <Arduino.h>
#include <functional>

// Callback-Typen für State-Änderungen
typedef std::function<void()> StateChangeCallback;
typedef std::function<void(uint32_t old_color, uint32_t new_color)> ColorChangeCallback;
typedef std::function<void(uint8_t old_brightness, uint8_t new_brightness)> BrightnessChangeCallback;
typedef std::function<void(const String& old_effect, const String& new_effect)> EffectChangeCallback;
typedef std::function<void(bool old_state, bool new_state)> PowerStateChangeCallback;

class StateManager {
public:
    // Singleton
    static StateManager& getInstance();
    
    // Power State
    void setPowerState(bool power);
    bool getPowerState() const { return power_state; }
    void togglePower();
    
    // Current Effect
    void setCurrentEffect(const String& effect);
    String getCurrentEffect() const { return current_effect; }
    
    // Color (RGB)
    void setColor(uint32_t rgb);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    uint32_t getColor() const { return color; }
    uint8_t getColorR() const { return (color >> 16) & 0xFF; }
    uint8_t getColorG() const { return (color >> 8) & 0xFF; }
    uint8_t getColorB() const { return color & 0xFF; }
    
    // Brightness (0-255)
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const { return brightness; }
    void increaseBrightness(uint8_t step = 10);
    void decreaseBrightness(uint8_t step = 10);
    
    // Speed/Tempo für Effekte (0-100)
    void setSpeed(uint8_t speed);
    uint8_t getSpeed() const { return speed; }

    // Additional tuning parameters
    void setIntensity(uint8_t intensity);
    uint8_t getIntensity() const { return intensity; }
    void setTransitionMs(uint16_t transition_ms);
    uint16_t getTransitionMs() const { return transition_ms; }

    // Color palette (0=Auto, 1=Warm, 2=Cool, 3=Natur, 4=Candy)
    void setPalette(uint8_t palette);
    uint8_t getPalette() const { return palette; }

    // Hue shift 0-359
    void setHueShift(uint16_t hue_shift);
    uint16_t getHueShift() const { return hue_shift; }
    
    // Callbacks registrieren
    void onPowerStateChange(PowerStateChangeCallback cb);
    void onColorChange(ColorChangeCallback cb);
    void onBrightnessChange(BrightnessChangeCallback cb);
    void onEffectChange(EffectChangeCallback cb);
    
    // Persistence
    void saveToPreferences();
    void scheduleSave(uint32_t delay_ms = 750);
    void processPendingSave();
    void loadFromPreferences();
    
    // Debug
    void printState() const;
    
private:
    StateManager() = default;
    
    // State Variables
    bool power_state = true;
    String current_effect = "clock";
    uint32_t color = 0xff9900;  // Orange
    uint8_t brightness = 120;
    uint8_t speed = 50;
    uint8_t intensity = 50;
    uint16_t transition_ms = 1000;
    uint8_t palette = 0;
    uint16_t hue_shift = 0;
    bool save_pending = false;
    unsigned long save_deadline_ms = 0;
    
    // Callbacks
    PowerStateChangeCallback on_power_change = nullptr;
    ColorChangeCallback on_color_change = nullptr;
    BrightnessChangeCallback on_brightness_change = nullptr;
    EffectChangeCallback on_effect_change = nullptr;
};

#endif
