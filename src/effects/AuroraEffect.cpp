// AuroraEffect.cpp
// Sanft schwingende horizontale Lichtvorhänge wie Polarlichter.
// Speed:     Driftgeschwindigkeit der Vorhänge
// Intensity: Sättigung / Farbtiefe (wenig = pastell, viel = kräftig)
// Color:     ignoriert – Aurora verwendet immer ihr eigenes Farbspektrum
//            (Blau, Grün, Violett, Türkis)

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

    // Offset driftet kontinuierlich
    const float driftStep = speedMapF(0.008f, 0.06f);
    _offset += driftStep;

    // Sättigung aus Intensity (80..240)
    const uint8_t sat = (uint8_t)intensityMap(80, 240);

    for (int y = 0; y < HEIGHT; y++) {
        // Jede Zeile: wellenförmige Helligkeit entlang X
        for (int x = 0; x < WIDTH; x++) {
            // Mehrere überlagerte Sinuswellen erzeugen organisches Rauschen
            float v  = sinf(x * 0.55f + _offset * 1.1f + y * 0.3f);
            v       += sinf(x * 0.30f - _offset * 0.7f + y * 0.5f) * 0.6f;
            v       += sinf((x + y) * 0.25f + _offset * 0.9f) * 0.4f;
            // v in [-2..2] → normieren auf [0..1]
            float t  = (v + 2.0f) / 4.0f;

            // Helligkeit: untere Hälfte dunkler (Vorhang-Fußpunkt)
            float yFade = 1.0f - (float)y / ((float)HEIGHT * 1.4f);
            float bright = t * yFade;
            if (bright < 0.0f) bright = 0.0f;

            // Farbton: Aurora-Palette – Grün(21845) über Türkis(32768) nach Blau/Violett(43690)
            // Variiert sanft mit x und Zeit
            float hueFrac = 0.33f + sinf(x * 0.2f + _offset * 0.5f) * 0.18f
                                  + sinf(y * 0.15f + _offset * 0.3f) * 0.10f;
            uint16_t hue = (uint16_t)(hueFrac * 65535.0f);  // Grün-Türkis-Blau-Violett

            uint8_t val = (uint8_t)(bright * 255.0f);
            if (val < 4) { strip->setPixelColor(XY(x, y), 0); continue; }

            uint32_t c = ledMatrix.colorHSV(hue, sat, val);
            strip->setPixelColor(XY(x, y),
                makeColorWithBrightness((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF));
        }
    }

    strip->show();
}
