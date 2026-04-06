#include "effect_helpers.h"

static bool sColorwipeDebugLogged = false;

void ColorwipeEffect::update() {
    if (!sColorwipeDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] colorwipe active");
        sColorwipeDebugLogged = true;
    }

    static unsigned long _lastFrameCW = 0;
    uint16_t frameDelay = speedToDelay(5, 100);
    if (millis() - _lastFrameCW < frameDelay) return;
    _lastFrameCW = millis();

    _hue += (uint16_t)intensityMap(5, 50);
    uint32_t c = ledMatrix.colorHSV(_hue, 255, 255);
    strip->setPixelColor(_pos, makeColorWithBrightness((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF));
    strip->show();
    _pos = (_pos + 1) % LED_PIXEL_AMOUNT;
}
