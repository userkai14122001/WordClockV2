#include "effect_helpers.h"

void StartupAnimation::run() {
    DebugManager::println(DebugCategory::Effects, "[FX] startup animation run");

    const uint16_t sweepDelay = speedToDelay(18, 90);
    const uint16_t holdDelay = speedToDelay(140, 520);
    const uint16_t fadeDelay = speedToDelay(8, 28);
    const uint16_t baseHue = hasUserColor() ? colorToHue16(color, 0) : 0;
    const uint16_t hueSpan = (uint16_t)densityMap(22000, 65535);
    const uint8_t sat = (uint8_t)intensityMap(180, 255);
    const uint8_t val = (uint8_t)intensityMap(150, 230);

    // Phase 1: Rainbow-Spalten sweepen von links nach rechts
    for (int x = 0; x < WIDTH; x++) {
        uint16_t hue = (uint16_t)(baseHue + ((uint32_t)x * hueSpan / WIDTH));
        uint32_t c   = ledMatrix.colorHSV(hue, sat, val);
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8)  & 0xFF;
        uint8_t b =  c        & 0xFF;
        for (int y = 0; y < HEIGHT; y++)
            strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
        strip->show();
        waitMs(sweepDelay);
    }

    // Phase 2: Kurz halten
    waitMs(holdDelay);

    // Phase 3: Ausblenden
    for (int step = 250; step >= 0; step -= 10) {
        float f = step / 255.0f;
        for (int x = 0; x < WIDTH; x++) {
            uint16_t hue = (uint16_t)(baseHue + ((uint32_t)x * hueSpan / WIDTH));
            uint32_t c   = ledMatrix.colorHSV(hue, sat, val);
            uint8_t r = (uint8_t)(((c >> 16) & 0xFF) * f);
            uint8_t g = (uint8_t)(((c >> 8)  & 0xFF) * f);
            uint8_t b = (uint8_t)(( c        & 0xFF) * f);
            for (int y = 0; y < HEIGHT; y++)
                ledMatrix.setPixelXYDirect(x, y, makeColorWithBrightness(r, g, b));
        }
        strip->show();
        waitMs(fadeDelay);
    }

    clearMatrix();
    strip->show();
    DebugManager::println(DebugCategory::Effects, "[FX] startup animation done");
}
