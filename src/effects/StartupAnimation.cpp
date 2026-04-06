#include "effect_helpers.h"

void StartupAnimation::run() {
    DebugManager::println(DebugCategory::Effects, "[FX] startup animation run");

    // Phase 1: Rainbow-Spalten sweepen von links nach rechts
    for (int x = 0; x < WIDTH; x++) {
        uint16_t hue = (uint16_t)((uint32_t)x * 65535 / WIDTH);
        uint32_t c   = ledMatrix.colorHSV(hue, 255, 230);
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8)  & 0xFF;
        uint8_t b =  c        & 0xFF;
        for (int y = 0; y < HEIGHT; y++)
            strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
        strip->show();
        waitMs(55);
    }

    // Phase 2: Kurz halten
    waitMs(400);

    // Phase 3: Ausblenden
    for (int step = 250; step >= 0; step -= 10) {
        float f = step / 255.0f;
        for (int x = 0; x < WIDTH; x++) {
            uint16_t hue = (uint16_t)((uint32_t)x * 65535 / WIDTH);
            uint32_t c   = ledMatrix.colorHSV(hue, 255, 230);
            uint8_t r = (uint8_t)(((c >> 16) & 0xFF) * f);
            uint8_t g = (uint8_t)(((c >> 8)  & 0xFF) * f);
            uint8_t b = (uint8_t)(( c        & 0xFF) * f);
            for (int y = 0; y < HEIGHT; y++)
                ledMatrix.setPixelXYDirect(x, y, makeColorWithBrightness(r, g, b));
        }
        strip->show();
        waitMs(18);
    }

    clearMatrix();
    strip->show();
    DebugManager::println(DebugCategory::Effects, "[FX] startup animation done");
}
