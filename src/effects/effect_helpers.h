#pragma once
// Shared helpers included by every effect .cpp in this folder.
// config.h (via main includes) already externs: color, brightness, currentEffect, powerState.
// matrix.h already externs: strip, ledMatrix.
#include <Arduino.h>
#include "config.h"
#include "matrix.h"
#include "effects.h"
#include "DebugManager.h"

// Variables defined in main.cpp / effects.cpp
extern uint8_t  effectSpeed;
extern uint8_t  effectIntensity;
extern uint8_t  effectDensity;   // 0-100, controls object count (balls, particles, stars, etc.)
extern uint16_t transitionMs;
extern uint8_t  effectPalette;   // 0=Auto 1=Warm 2=Cool 3=Natur 4=Candy
extern uint16_t effectHueShift;  // 0-359 degrees

// ------------------------------------
// Speed / Intensity helpers
// ------------------------------------
static inline uint8_t clampSpeed() {
    return effectSpeed < 1 ? 1 : (effectSpeed > 100 ? 100 : effectSpeed);
}
static inline uint8_t clampIntensity() {
    return effectIntensity < 1 ? 1 : (effectIntensity > 100 ? 100 : effectIntensity);
}
static inline uint16_t speedToDelay(uint16_t minMs, uint16_t maxMs) {
    uint8_t s = clampSpeed();
    return maxMs - (uint32_t)(maxMs - minMs) * (s - 1) / 99;
}
static inline int32_t speedMap(int32_t minVal, int32_t maxVal) {
    uint8_t s = clampSpeed();
    return minVal + (int32_t)(maxVal - minVal) * (s - 1) / 99;
}
static inline float speedMapF(float minVal, float maxVal) {
    uint8_t s = clampSpeed();
    return minVal + (maxVal - minVal) * (float)(s - 1) / 99.0f;
}
static inline int32_t intensityMap(int32_t minVal, int32_t maxVal) {
    uint8_t i = clampIntensity();
    return minVal + (int32_t)(maxVal - minVal) * (i - 1) / 99;
}
static inline float intensityMapF(float minVal, float maxVal) {
    uint8_t i = clampIntensity();
    return minVal + (maxVal - minVal) * (float)(i - 1) / 99.0f;
}
static inline uint8_t clampDensity() {
    return effectDensity < 1 ? 1 : (effectDensity > 100 ? 100 : effectDensity);
}
static inline int32_t densityMap(int32_t minVal, int32_t maxVal) {
    uint8_t d = clampDensity();
    return minVal + (int32_t)(maxVal - minVal) * (d - 1) / 99;
}
static inline float densityMapF(float minVal, float maxVal) {
    uint8_t d = clampDensity();
    return minVal + (maxVal - minVal) * (float)(d - 1) / 99.0f;
}
static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) yield();
}

// ------------------------------------
// Color channel helpers
// ------------------------------------
static inline uint8_t colR(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t colG(uint32_t c) { return (c >> 8)  & 0xFF; }
static inline uint8_t colB(uint32_t c) { return  c        & 0xFF; }
static inline bool hasUserColor() { return color != 0; }
static inline uint16_t colorToHue16(uint32_t rgb, uint16_t fallbackHue = 0) {
    uint8_t r = colR(rgb), g = colG(rgb), b = colB(rgb);
    uint8_t maxCh = max(r, max(g, b));
    uint8_t minCh = min(r, min(g, b));
    if (maxCh == minCh) return fallbackHue;

    int32_t delta = (int32_t)maxCh - (int32_t)minCh;
    int32_t hueDeg;
    if (maxCh == r) {
        hueDeg = ((int32_t)(g - b) * 60) / delta;
        if (g < b) hueDeg += 360;
    } else if (maxCh == g) {
        hueDeg = ((int32_t)(b - r) * 60) / delta + 120;
    } else {
        hueDeg = ((int32_t)(r - g) * 60) / delta + 240;
    }

    return (uint16_t)((uint32_t)(((hueDeg % 360) + 360) % 360) * 65535UL / 360UL);
}
