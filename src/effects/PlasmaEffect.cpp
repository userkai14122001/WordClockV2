#include "effect_helpers.h"

static bool sPlasmaDebugLogged = false;

void PlasmaEffect::update() {
    if (!sPlasmaDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] plasma active");
        sPlasmaDebugLogged = true;
    }

    uint16_t frameInterval = speedToDelay(15, 100);
    if (millis() - _last < frameInterval) return;
    _last = millis();

    _t++;
    float speedMult = speedMapF(0.3f, 1.5f);

    uint8_t sat = (uint8_t)intensityMap(140, 255);
    uint8_t val = (uint8_t)intensityMap(180, 255);

    float fT = (float)_t * speedMult;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float v = 0;
            v += sinf(x * 0.70f + fT * 0.11f);
            v += sinf(y * 0.55f + fT * 0.09f);
            v += sinf((x + y) * 0.40f + fT * 0.07f);
            float pcx = x - WIDTH  / 2.0f;
            float pcy = y - HEIGHT / 2.0f;
            v += sinf(sqrtf(pcx*pcx + pcy*pcy) * 0.75f + fT * 0.13f);
            uint16_t hue = (uint16_t)(((v + 4.0f) / 8.0f) * 65535.0f);
            uint32_t c = ledMatrix.colorHSV(hue, sat, val);
            strip->setPixelColor(XY(x, y),
                makeColorWithBrightness((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF));
        }
    }

    strip->show();
}
