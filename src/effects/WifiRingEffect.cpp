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

    if (millis() - _lastFrame < 60) return;
    _lastFrame = millis();

    uint32_t col = _fixedColor ? _fixedColor : color;
    clearMatrix();

    uint8_t R = colR(col), G = colG(col), B = colB(col);

    // _ringStep > 0 means draw multiple rings (e.g. every second ring in setup mode).
    int startRing = _ring;
    int endRing = (_ringStep > 0) ? 10 : (_ring + 1);
    int step = (_ringStep > 0) ? _ringStep : 1;

    for (int r = startRing; r < endRing; r += step) {
        int total = ringLength(r);
        if (total <= 0) break;
        int trail = max(1, total / 4);

        for (int i = 0; i < trail; i++) {
            float fade = 1.0f - (float)i / trail;
            uint32_t c = makeColorWithBrightness(R * fade, G * fade, B * fade);
            int pos = (_headPos[r] - i + total) % total;
            setRingPixel(r, pos, c);
        }

        _headPos[r] = (_headPos[r] + 1) % total;
    }

    strip->show();
}
