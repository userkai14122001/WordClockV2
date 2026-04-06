#include "effect_helpers.h"

static bool sWaterDropDebugLogged = false;

void WaterDropEffect::update() {
    if (!sWaterDropDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] waterdrop active");
        sWaterDropDebugLogged = true;
    }

    const uint16_t frameDelay = speedToDelay(10, 150);
    const float ringWidth     = 1.0f + intensityMap(0, 40) * 0.1f;
    const float radiusStep    = speedMapF(0.05f, 0.35f);

    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    uint8_t colR_val = colR(color);
    uint8_t colG_val = colG(color);
    uint8_t colB_val = colB(color);

    const float cx         = (WIDTH  - 1) / 2.0f;
    const float cy         = (HEIGHT - 1) / 2.0f;
    const float maxDist    = sqrt(cx*cx + cy*cy);
    const float startRadius = -0.35f;

    clearMatrix();

    // Ring starts narrow in the center and grows to full configured width
    float growT = (_radius - startRadius) / 2.0f;
    if (growT < 0.0f) growT = 0.0f;
    if (growT > 1.0f) growT = 1.0f;
    const float currentRingWidth = 0.35f + (ringWidth - 0.35f) * growT;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float dx   = x - cx, dy = y - cy;
            float dist = sqrt(dx*dx + dy*dy);
            float diff = fabs(dist - _radius);
            if (diff < currentRingWidth) {
                float fade = 1.0f - diff / currentRingWidth;
                strip->setPixelColor(XY(x, y),
                    makeColorWithBrightness(colR_val * fade, colG_val * fade, colB_val * fade));
            }
        }
    }

    strip->show();
    _radius += radiusStep;

    const float endRadius = maxDist + currentRingWidth + 0.8f;

    if (_singleMode) {
        if (_radius > endRadius) {
            DebugManager::println(DebugCategory::Effects, "[FX][waterdrop] single-shot done -> clock");
            _radius = startRadius;
            currentEffect = "clock";
        }
        return;
    }
    if (_radius > endRadius) _radius = startRadius;
}
