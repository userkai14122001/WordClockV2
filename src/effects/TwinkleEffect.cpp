// TwinkleEffect.cpp
// Zufällige Pixel flackern auf und ab – wie ein Sternenhimmel.
// Speed:     wie schnell die Sterne pulsieren
// Intensity: Helligkeit und Glitzer-Stärke der Sterne
// Density:   wie viele Sterne gleichzeitig sichtbar sind
// Color:     Grundfarbe; mit leichter Farbton-Variation pro Stern

#include "effect_helpers.h"

static bool sTwinkleDebugLogged = false;

void TwinkleEffect::update() {
    if (!sTwinkleDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] twinkle active");
        sTwinkleDebugLogged = true;
    }

    const uint16_t frameDelay = speedToDelay(20, 120);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    // Anzahl aktiver Sterne: density 1-100 → 3..MAX_STARS
    const int starCount = 3 + (int)densityMap(0, MAX_STARS - 3);
    const float intensityScale = intensityMapF(0.25f, 1.00f);
    const uint8_t rainbowSat = (uint8_t)intensityMap(160, 255);

    // Beim ersten Aufruf oder nach reset(): Sterne initialisieren
    // Erkennungsmerkmal: speed == 0 bedeutet uninitialisiert
    static bool _seeded = false;
    if (!_seeded) {
        for (int i = 0; i < MAX_STARS; i++) {
            _stars[i].x     = random(0, WIDTH);
            _stars[i].y     = random(0, HEIGHT);
            _stars[i].phase = random(0, 256);
            _stars[i].speed = 2 + random(0, 8);
            _stars[i].hue   = random(0, 65536);
        }
        _seeded = true;
    }

    clearMatrix();

    uint8_t baseR = colR(color);
    uint8_t baseG = colG(color);
    uint8_t baseB = colB(color);
    bool useColor = (color != 0);  // 0 = rainbow-Modus

    for (int i = 0; i < starCount; i++) {
        TwinkleStar& s = _stars[i];

        // Phase vorwärts schieben; speed-Skalar aus globalem speed-Slider
        uint8_t phaseStep = (uint8_t)(s.speed * speedMapF(0.5f, 2.5f));
        s.phase += phaseStep;

        // Sinus auf [0..1] mappen; sin8 liefert 0..255 für eine volle Welle
        // Wir nutzen (sin(phase)+1)/2 als Helligkeit
        float bright = (sinf(s.phase * 0.02454f) + 1.0f) * 0.5f;  // 0.02454 = 2π/256
        bright *= intensityScale;
        if (bright > 1.0f) bright = 1.0f;

        if (bright < 0.05f) {
            // Stern ist "aus" → neue zufällige Position vergeben
            s.x     = random(0, WIDTH);
            s.y     = random(0, HEIGHT);
            s.speed = 2 + random(0, 8);
            s.hue   = (uint16_t)(color ? (_stars[i].hue + random(-2000, 2000)) : random(0, 65536));
        }

        uint8_t r, g, b;
        if (useColor) {
            // Grundfarbe aus color-Variable + leichter Aufhellung durch bright
            r = (uint8_t)(baseR * bright);
            g = (uint8_t)(baseG * bright);
            b = (uint8_t)(baseB * bright);
            // Füge etwas Weiß hinzu je heller der Stern (Glitzereffekt)
            uint8_t glitter = (uint8_t)(bright * bright * 80);
            r = min(255, (int)r + glitter);
            g = min(255, (int)g + glitter);
            b = min(255, (int)b + glitter);
        } else {
            // Rainbow-Modus: jeder Stern hat eigenen Farbton
            uint32_t c = ledMatrix.colorHSV(s.hue, rainbowSat, (uint8_t)(bright * 255));
            r = (c >> 16) & 0xFF;
            g = (c >> 8)  & 0xFF;
            b =  c        & 0xFF;
        }

        strip->setPixelColor(XY(s.x, s.y), makeColorWithBrightness(r, g, b));
    }

    strip->show();
}
