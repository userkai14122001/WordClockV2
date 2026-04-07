#include "effect_helpers.h"

static bool sWifiRingDebugLogged = false;

// ---------------------------------------------------------
// Ring geometry helpers (only needed by this effect)
// ---------------------------------------------------------
static int ringLength(int ring) {
    int w = WIDTH  - 2*ring;
    int h = HEIGHT - 2*ring;
    return 2*(w + h) - 4;
}

static void setRingPixel(int ring, int index, uint32_t c) {
    int o = ring;
    int w = WIDTH  - 2*o;
    int h = HEIGHT - 2*o;
    int total = ringLength(ring);

    index %= total;

    if (index < w) { strip->setPixelColor(XY(o + index, o), c); return; }
    index -= w;

    if (index < h - 1) { strip->setPixelColor(XY(o + w - 1, o + 1 + index), c); return; }
    index -= (h - 1);

    if (index < w - 1) { strip->setPixelColor(XY(o + w - 2 - index, o + h - 1), c); return; }
    index -= (w - 1);

    strip->setPixelColor(XY(o, o + h - 2 - index), c);
}

// ---------------------------------------------------------
// WifiRingEffect::update()
// ---------------------------------------------------------
void WifiRingEffect::update() {
    if (!sWifiRingDebugLogged) {
        DebugManager::printf(DebugCategory::Effects, "[FX] wifi ring active (ring=%d step=%d)\n", _ring, _ringStep);
        sWifiRingDebugLogged = true;
    }

    const uint16_t frameDelay = speedToDelay(25, 120);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    // Density selects which ring to animate (0=outer perimeter ... 4=innermost)
    const int activeRing = (_ringStep > 0) ? _ring : (int)densityMap(0, 4);

    uint32_t col = _fixedColor ? _fixedColor : color;
    clearMatrix();

    uint8_t R = colR(col), G = colG(col), B = colB(col);
    if (col == 0) {
        R = 90; G = 170; B = 255;
    }
    // Intensity: head brightness AND trail length (high = bright long comet)
    const float headScale = intensityMapF(0.45f, 1.00f);

    // _ringStep > 0 means draw multiple rings (e.g. every second ring in setup mode).
    int startRing = activeRing;
    int endRing = (_ringStep > 0) ? 10 : (activeRing + 1);
    int step = (_ringStep > 0) ? _ringStep : 1;

    for (int r = startRing; r < endRing; r += step) {
        int total = ringLength(r);
        if (total <= 0) break;
        int trail = max(1, (int)(total * intensityMapF(0.12f, 0.45f)));

        for (int i = 0; i < trail; i++) {
            float fade = 1.0f - (float)i / trail;
            float bright = fade * headScale;
            uint32_t c = makeColorWithBrightness(R * bright, G * bright, B * bright);
            int pos = (_headPos[r] - i + total) % total;
            setRingPixel(r, pos, c);
        }

        _headPos[r] = (_headPos[r] + 1) % total;
    }

    strip->show();
}
