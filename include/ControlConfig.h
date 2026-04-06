#pragma once

#include <Arduino.h>

namespace ControlConfig {
// Defaults
static constexpr uint8_t DEFAULT_BRIGHTNESS = 120;
static constexpr uint32_t DEFAULT_COLOR = 0xFF9900;
static constexpr uint8_t DEFAULT_SPEED = 50;
static constexpr uint8_t DEFAULT_INTENSITY = 50;
static constexpr uint16_t DEFAULT_TRANSITION_MS = 1000;
static constexpr uint16_t CLOCK_MORPH_MAX_MS = 1200;

// Ranges
static constexpr uint8_t BRIGHTNESS_MIN = 0;
static constexpr uint8_t BRIGHTNESS_MAX = 255;

static constexpr uint8_t SPEED_MIN = 1;
static constexpr uint8_t SPEED_MAX = 100;

static constexpr uint8_t INTENSITY_MIN = 1;
static constexpr uint8_t INTENSITY_MAX = 100;

static constexpr uint16_t TRANSITION_MIN_MS = 200;
static constexpr uint16_t TRANSITION_MAX_MS = 10000;

// Effect palette (0=Auto, 1=Warm, 2=Cool, 3=Natur, 4=Candy)
static constexpr uint8_t DEFAULT_PALETTE  = 0;
static constexpr uint8_t PALETTE_MIN      = 0;
static constexpr uint8_t PALETTE_MAX      = 4;

// Hue shift 0-359 degrees applied to HSV-based effects
static constexpr uint16_t DEFAULT_HUE_SHIFT = 0;
static constexpr uint16_t HUE_SHIFT_MIN     = 0;
static constexpr uint16_t HUE_SHIFT_MAX     = 359;

// MQTT color anti-jitter timings
static constexpr uint16_t MQTT_COLOR_SETTLE_MS = 50;
static constexpr uint16_t MQTT_COLOR_MIN_APPLY_INTERVAL_MS = 10;
}
