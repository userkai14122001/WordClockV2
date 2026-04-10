#include "effect_helpers.h"

static bool sInwardDebugLogged = false;

void InwardRippleEffect::update() {
    if (!sInwardDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] waterdrop_r active");
        sInwardDebugLogged = true;
    }

    const uint16_t frameDelay = speedToDelay(10, 150);
    const float ringWidth     = 1.0f + intensityMap(0, 40) * 0.1f;
    const float radiusStep    = speedMapF(0.05f, 0.35f);
    const float edgeSoftness  = densityMapF(0.85f, 1.65f);

    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    uint8_t colR_val = hasUserColor() ? colR(color) : 255;
    uint8_t colG_val = hasUserColor() ? colG(color) : 255;
    uint8_t colB_val = hasUserColor() ? colB(color) : 255;

    const float cx      = (WIDTH  - 1) / 2.0f;
    const float cy      = (HEIGHT - 1) / 2.0f;
    const float maxDist = sqrtf(cx*cx + cy*cy);

    // Radius starts at the outer edge and collapses inward
    if (_radius < 0.0f) _radius = maxDist + 0.35f;

    clearMatrix();

    // Ring width grows as it approaches centre (inverse of outward drop)
    float shrinkT = 1.0f - (_radius / (maxDist + 0.35f));
    if (shrinkT < 0.0f) shrinkT = 0.0f;
    if (shrinkT > 1.0f) shrinkT = 1.0f;
    const float currentRingWidth = 0.35f + (ringWidth - 0.35f) * shrinkT;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float dx   = x - cx, dy = y - cy;
            float dist = sqrtf(dx*dx + dy*dy);
            float diff = fabsf(dist - _radius);
            float effectiveWidth = currentRingWidth * edgeSoftness;
            if (diff < effectiveWidth) {
                float fade = 1.0f - diff / effectiveWidth;
                strip->setPixelColor(XY(x, y),
                    makeColorWithBrightness(colR_val * fade, colG_val * fade, colB_val * fade));
            }
        }
    }

    strip->show();
    _radius -= radiusStep;

    if (_radius < -(currentRingWidth + 0.8f)) _radius = maxDist + 0.35f;
}
