#include "effect_helpers.h"
#include <math.h>

#include "effects.h"

static bool sLoveDebugLogged = false;

void LoveYouEffect::update() {
    if (!sLoveDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] love active");
        sLoveDebugLogged = true;
    }

    // Speed controls breathing cadence: slow pulse at 1, fast shimmer at 100
    const uint16_t frameDelay = speedToDelay(30, 200);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    // Phase advances fast at high speed, slow at low speed
    _phase += speedMapF(0.025f, 0.16f);

    // Intensity sets the brightness ceiling (low = subtle glow, high = full brightness)
    float maxBright = intensityMapF(0.30f, 1.0f);

    // Breathing envelope: 0.3 (dark) .. 1.0 (full) * maxBright
    float sine   = (sinf(_phase) + 1.0f) * 0.5f;  // 0..1
    float bright = (0.30f + 0.70f * sine) * maxBright;

    uint8_t r, g, b;
    if (hasUserColor()) {
        r = (uint8_t)(colR(color) * bright);
        g = (uint8_t)(colG(color) * bright);
        b = (uint8_t)(colB(color) * bright);
    } else {
        // Auto: warm rose/pink
        r = (uint8_t)(255 * bright);
        g = (uint8_t)( 25 * bright);
        b = (uint8_t)( 85 * bright);
    }

    uint32_t c = makeColorWithBrightness(r, g, b);

    clearMatrix();
    Word wLove, wYou;
    if (!getClockWordPosition("LOVE", wLove)) wLove = {3, 3, 4};
    if (!getClockWordPosition("YOU",  wYou))  wYou  = {4, 5, 3};
    for (int i = 0; i < wLove.len; i++)
        strip->setPixelColor(XY(wLove.x + i, wLove.y), c);
    for (int i = 0; i < wYou.len; i++)
        strip->setPixelColor(XY(wYou.x + i, wYou.y), c);
    strip->show();
}

