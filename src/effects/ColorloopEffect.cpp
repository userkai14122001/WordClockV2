#include "effect_helpers.h"

static bool sColorloopDebugLogged = false;

void ColorloopEffect::update() {
    if (!sColorloopDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] colorloop active");
        sColorloopDebugLogged = true;
    }

    static unsigned long _lastFrameCL = 0;
    uint16_t frameDelay = speedToDelay(10, 80);
    if (millis() - _lastFrameCL < frameDelay) return;
    _lastFrameCL = millis();

    uint16_t hueStep = (uint16_t)speedMap(2, 20);
    _baseHue += hueStep;

    uint16_t spread = (uint16_t)intensityMap(80, 800);

    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        uint32_t c = ledMatrix.colorHSV(_baseHue + i * spread, 255, 255);
        strip->setPixelColor(i, makeColorWithBrightness((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF));
    }
    strip->show();
}
