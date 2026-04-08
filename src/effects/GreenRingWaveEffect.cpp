#include "effect_helpers.h"

// ---------------------------------------------------------
// Ring geometry helpers
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
// GreenRingWaveEffect::update()
// Grüne Welle läuft durch Ring-Ebenen nach innen
// ---------------------------------------------------------
void GreenRingWaveEffect::update() {
    const uint16_t frameDelay = speedToDelay(40, 180);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    clearMatrix();

    // Grün: RGB(0, 255, 0)
    uint32_t greenBright = makeColorWithBrightness(0, 255, 0);
    uint32_t greenDim = makeColorWithBrightness(0, 100, 0);

    // Suche die maximale Ring-Ebene
    int maxRings = 0;
    for (int r = 0; r < 10; r++) {
        if (ringLength(r) > 0) maxRings = r + 1;
        else break;
    }

    // Aktuelle Position: welche Ring-Ebene und welcher Pixel in dieser Ebene
    int cycleLen = 0;
    for (int r = 0; r < maxRings; r++) {
        cycleLen += ringLength(r);
    }

    if (cycleLen <= 0) {
        clearMatrix();
        return;
    }

    int pos = _wavePos % cycleLen;

    // Zeichne die Welle durch die Ringe
    int currentPos = 0;
    for (int r = 0; r < maxRings; r++) {
        int rLen = ringLength(r);
        int rStart = currentPos;
        int rEnd = currentPos + rLen;

        if (pos >= rStart && pos < rEnd) {
            // Grüner Punkt in diesem Ring
            int pixelInRing = pos - rStart;
            setRingPixel(r, pixelInRing, greenBright);

            // Trail auf diesem Ring (Schweif)
            int trail = max(2, rLen / 8);
            for (int i = 1; i < trail; i++) {
                float fade = 1.0f - (float)i / trail;
                uint32_t c = makeColorWithBrightness(0, (uint8_t)(100 * fade), 0);
                int trailPixel = (pixelInRing - i + rLen) % rLen;
                setRingPixel(r, trailPixel, c);
            }
        }

        currentPos = rEnd;
    }

    _wavePos++;
}
