// AuroraEffect.cpp
// Sanft schwingende horizontale Lichtvorhänge wie Polarlichter.
// Speed:     Driftgeschwindigkeit der Vorhänge
// Intensity: Helligkeit und Kontrast der Vorhänge
// Density:   Detailgrad / Anzahl sichtbarer Wellenstrukturen
// Color:     optionaler Farbanker; 0 = klassisches Aurora-Spektrum

#include "effect_helpers.h"

static bool sAuroraDebugLogged = false;

void AuroraEffect::update() {
    if (!sAuroraDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] aurora active");
        sAuroraDebugLogged = true;
    }

    const uint16_t frameDelay = speedToDelay(30, 200);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    const float detailScale = densityMapF(0.80f, 1.45f);
    const float brightScale = intensityMapF(0.70f, 1.30f);

    // Offset driftet kontinuierlich
    const float driftStep = speedMapF(0.008f, 0.06f);
    _offset += driftStep;

    const bool userColor = hasUserColor();
    const uint16_t baseHue = userColor ? colorToHue16(color, 21845U) : 21845U;
    const float hueBase = (float)baseHue / 65535.0f;
    const float hueSwingX = densityMapF(0.08f, 0.22f);
    const float hueSwingY = densityMapF(0.04f, 0.12f);
    const uint8_t sat = userColor ? (uint8_t)intensityMap(190, 255) : 255;

    for (int y = 0; y < HEIGHT; y++) {
        // Jede Zeile: wellenförmige Helligkeit entlang X
        for (int x = 0; x < WIDTH; x++) {
            // Mehrere überlagerte Sinuswellen erzeugen organisches Rauschen
            float v  = sinf(x * (0.42f + detailScale * 0.18f) + _offset * 1.1f + y * 0.3f);
            v       += sinf(x * (0.22f + detailScale * 0.14f) - _offset * 0.7f + y * 0.5f) * 0.6f;
            v       += sinf((x + y) * (0.18f + detailScale * 0.10f) + _offset * 0.9f) * 0.4f;
            // v in [-2..2] → normieren auf [0..1]
            float t  = (v + 2.0f) / 4.0f;

            // Helligkeit: untere Hälfte dunkler (Vorhang-Fußpunkt)
            float yFade = 1.0f - (float)y / ((float)HEIGHT * 1.4f);
            float bright = t * yFade * brightScale;
            if (bright < 0.0f) bright = 0.0f;
            if (bright > 1.0f) bright = 1.0f;

            float hueFrac = hueBase + sinf(x * 0.2f + _offset * 0.5f) * hueSwingX
                                    + sinf(y * 0.15f + _offset * 0.3f) * hueSwingY;
            if (!userColor) {
                hueFrac += 0.04f;
            }
            while (hueFrac < 0.0f) hueFrac += 1.0f;
            while (hueFrac > 1.0f) hueFrac -= 1.0f;
            uint16_t hue = (uint16_t)(hueFrac * 65535.0f);

            uint8_t val = (uint8_t)(bright * 255.0f);
            if (val < 4) { strip->setPixelColor(XY(x, y), 0); continue; }

            uint32_t c = ledMatrix.colorHSV(hue, sat, val);
            strip->setPixelColor(XY(x, y),
                makeColorWithBrightness((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF));
        }
    }

    strip->show();
}
