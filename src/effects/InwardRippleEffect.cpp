#include "effect_helpers.h"

static bool sInwardDebugLogged = false;

void InwardRippleEffect::update() {
    if (!sInwardDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] inward active");
        sInwardDebugLogged = true;
    }

    const uint16_t frameDelay = speedToDelay(18, 95);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    const float cx      = (WIDTH  - 1) / 2.0f;
    const float cy      = (HEIGHT - 1) / 2.0f;
    const float maxDist = sqrtf(cx*cx + cy*cy);

    // Weniger gleichzeitig aktive Wellen, dafuer weicher und gleichmaessiger.
    const int   numWaves    = 1 + (int)densityMap(0, 2);
    const float waveSpacing = (maxDist + 1.8f) / (float)numWaves;
    const float baseWidth   = intensityMapF(0.70f, 1.80f);
    const float radiusStep  = speedMapF(0.05f, 0.18f);
    const float waveBoost   = intensityMapF(0.70f, 1.25f);
    const float ambientBase = intensityMapF(0.02f, 0.08f);

    if (_radius < 0.0f) _radius = maxDist + 1.0f;

    clearMatrix();

    uint8_t R = colR(color), G = colG(color), B = colB(color);
    if (color == 0) {
        R = 40;
        G = 140;
        B = 255;
    }

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float dx   = x - cx, dy = y - cy;
            float dist = sqrtf(dx*dx + dy*dy);

            float waveLight = 0.0f;
            for (int w = 0; w < numWaves; w++) {
                float waveRadius = _radius - w * waveSpacing;
                if (waveRadius < -1.6f) continue;

                float clampedRadius = constrain(waveRadius, 0.0f, maxDist);
                float proximity     = 1.0f - clampedRadius / maxDist;
                float wWidth        = baseWidth + proximity * 0.9f;

                float diff  = fabsf(dist - waveRadius);
                float sigma = max(0.55f, wWidth * 0.65f);
                float fade  = expf(-(diff * diff) / (2.0f * sigma * sigma));
                fade *= (0.30f + proximity * 0.55f) * waveBoost;
                waveLight += fade;
            }

            float centerGlow = 1.0f - dist / (maxDist + 0.01f);
            centerGlow = max(0.0f, centerGlow);
            centerGlow = centerGlow * centerGlow * intensityMapF(0.08f, 0.24f);

            float ambient = ambientBase + centerGlow;
            float total   = min(1.0f, ambient + waveLight * 0.85f);

            strip->setPixelColor(
                XY(x, y),
                makeColorWithBrightness(
                    (uint8_t)(R * total),
                    (uint8_t)(G * total),
                    (uint8_t)(B * total)
                )
            );
        }
    }

    strip->show();

    _radius -= radiusStep;
    if (_radius < -waveSpacing) {
        _radius = maxDist + waveSpacing;
    }
}
