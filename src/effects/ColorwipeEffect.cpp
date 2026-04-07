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

    _hue += (uint16_t)densityMap(3, 90);
    uint16_t hue = hasUserColor() ? (uint16_t)(colorToHue16(color, _hue) + _hue) : _hue;
    uint8_t sat = (uint8_t)intensityMap(150, 255);
    uint8_t val = (uint8_t)intensityMap(135, 255);
    uint32_t c = ledMatrix.colorHSV(hue, sat, val);
    strip->setPixelColor(_pos, makeColorWithBrightness((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF));
    strip->show();
    _pos = (_pos + 1) % LED_PIXEL_AMOUNT;
}
