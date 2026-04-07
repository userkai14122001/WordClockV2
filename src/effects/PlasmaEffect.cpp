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
    float detail = densityMapF(0.80f, 1.45f);

    uint8_t sat = hasUserColor() ? 255 : 230;
    uint8_t val = (uint8_t)intensityMap(160, 255);
    uint16_t hueBias = hasUserColor() ? colorToHue16(color, 0) : 0;

    float fT = (float)_t * speedMult;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float v = 0;
            v += sinf(x * (0.50f + detail * 0.20f) + fT * 0.11f);
            v += sinf(y * (0.38f + detail * 0.17f) + fT * 0.09f);
            v += sinf((x + y) * (0.24f + detail * 0.16f) + fT * 0.07f);
            float pcx = x - WIDTH  / 2.0f;
            float pcy = y - HEIGHT / 2.0f;
            v += sinf(sqrtf(pcx*pcx + pcy*pcy) * (0.50f + detail * 0.25f) + fT * 0.13f);
            uint16_t hue = (uint16_t)(hueBias + (((v + 4.0f) / 8.0f) * 65535.0f));
            uint32_t c = ledMatrix.colorHSV(hue, sat, val);
            strip->setPixelColor(XY(x, y),
                makeColorWithBrightness((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF));
        }
    }

    strip->show();
}
