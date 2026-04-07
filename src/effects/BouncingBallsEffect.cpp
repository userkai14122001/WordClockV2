// BouncingBallsEffect.cpp
// Bälle prallen physikalisch von den Wänden ab. Jeder Ball hinterlässt
// einen kurz nachglühenden Schweif-Trail.
// Speed:     Bewegungsgeschwindigkeit der Bälle
// Intensity: Trail-Stärke und Nachleuchten
// Density:   Anzahl der Bälle (2..MAX_BALLS)
// Color:     0 = jeder Ball eigene Farbe; sonst Grundfarbe mit leichten Hue-Offsets

#include "effect_helpers.h"
#include "DebugManager.h"

// Trail-Buffer wird per Lazy-Allocation in update() allokiert (~110 Bytes bei Bedarf)

static void respawnBallFromTop(Ball& b, int idx, float speedScale) {
    b.x = random(0, WIDTH);
    b.y = -0.3f;
    b.vx = speedMapF(-0.28f, 0.28f) * speedScale;
    b.vy = speedMapF(0.18f, 0.50f) * speedScale;
    b.hue = (uint16_t)((idx * (65535 / 8) + random(0, 5000)) & 0xFFFF);
    b.energy = 70.0f + (float)random(0, 31);  // 70..100
}

void BouncingBallsEffect::update() {
    static unsigned long gravityPhaseStart = 0;

    const uint16_t frameDelay = speedToDelay(16, 80);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    const int ballCount = 2 + (int)densityMap(0, MAX_BALLS - 2);
    const float speedScale = speedMapF(0.3f, 1.4f);
    const float gravityY = speedMapF(0.020f, 0.060f);
    const uint8_t trailDecay = (uint8_t)intensityMap(100, 28);
    const uint8_t trailBase = (uint8_t)intensityMap(100, 165);
    const uint8_t trailSat = (uint8_t)intensityMap(160, 255);
    const bool useColor = hasUserColor();
    const uint16_t baseHue = colorToHue16(color, 0);

    if (gravityPhaseStart == 0) gravityPhaseStart = millis();
    float phase = (millis() - gravityPhaseStart) * 0.0017f;
    float gravityX = sinf(phase) * speedMapF(0.003f, 0.014f);

    // Initialisierung + Trail-Allokation (einmalig beim ersten Aufruf)
    if (!_seeded) {
        // Lazy-Allocation des Trail-Buffers (nur einmal)
        if (!_trail) {
            _trail = new uint8_t[HEIGHT * WIDTH];
            if (!_trail) {
                // Fehlerfall: zu wenig Speicher, Effekt abbrechen
                DebugManager::println(DebugCategory::Effects, "ERROR: BouncingBalls trail alloc failed");
                return;
            }
        }
        memset(_trail, 0, HEIGHT * WIDTH);

        for (int i = 0; i < MAX_BALLS; i++) {
            _balls[i].x   = random(0, WIDTH);
            _balls[i].y   = random(0, HEIGHT);
            // Zufällige Richtung, normiert auf speedScale
            float angle   = random(0, 628) * 0.01f;  // 0..2π
            _balls[i].vx  = cosf(angle) * speedScale;
            _balls[i].vy  = sinf(angle) * speedScale;
            _balls[i].hue = (uint16_t)(i * (65535 / MAX_BALLS));
            _balls[i].energy = 70.0f + (float)random(0, 31);
        }
        memset(_trail, 0, HEIGHT * WIDTH);
        _seeded = true;
    }

    // Trail deutlich schneller verblassen (kurzer Schweif)
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            int idx = y * WIDTH + x;
            if (_trail[idx] > trailDecay) _trail[idx] -= trailDecay;
            else                  _trail[idx]  = 0;
        }

    // Bälle bewegen
    for (int i = 0; i < ballCount; i++) {
        Ball& b = _balls[i];

        // Energie sinkt leicht über Zeit
        b.energy -= speedMapF(0.20f, 0.55f);
        if (b.energy <= 0.0f) {
            respawnBallFromTop(b, i, speedScale);
        }

        // Gravitation wirkt permanent nach unten und leicht seitlich pendelnd.
        b.vx += gravityX;
        b.vy += gravityY;

        // Begrenze Maximalgeschwindigkeit für ruhigeres Bounce-Verhalten.
        b.vx = constrain(b.vx, -1.20f, 1.20f);
        b.vy = constrain(b.vy, -1.70f, 1.70f);

        b.x += b.vx;
        b.y += b.vy;

        // Abprallen von Wänden
        if (b.x < 0.0f) {
            b.x = 0.0f;
            b.vx = fabsf(b.vx) * 0.80f;
            b.vy *= 0.95f;
            b.energy -= 8.0f;
        }
        if (b.x >= (float)WIDTH) {
            b.x = WIDTH - 1.0f;
            b.vx = -fabsf(b.vx) * 0.80f;
            b.vy *= 0.95f;
            b.energy -= 8.0f;
        }
        if (b.y < 0.0f) {
            b.y = 0.0f;
            b.vy = fabsf(b.vy) * 0.72f;
            b.vx *= 0.94f;
            b.energy -= 7.0f;
        }
        if (b.y >= (float)HEIGHT) {
            b.y = HEIGHT - 1.0f;
            b.vy = -fabsf(b.vy) * 0.66f;
            b.vx *= 0.92f;
            b.energy -= 16.0f;
        }

        if (b.energy <= 0.0f) {
            respawnBallFromTop(b, i, speedScale);
        }

        // Trail-Spur hinterlassen
        int px = (int)(b.x + 0.5f);
        int py = (int)(b.y + 0.5f);
        if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
            _trail[py * WIDTH + px] = (uint8_t)min(255, trailBase + (int)(b.energy * 1.1f));
    }

    // Rendern: Trail + Ballköpfe
    clearMatrix();

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int idx = y * WIDTH + x;
            if (_trail[idx] == 0) continue;
            float fade = _trail[idx] / 255.0f;

            // Welchem Ball gehört dieser Pixel (nächster Ball)
            int nearest = 0;
            float minDist = 9999.0f;
            for (int i = 0; i < ballCount; i++) {
                float dx = x - _balls[i].x, dy = y - _balls[i].y;
                float d  = dx*dx + dy*dy;
                if (d < minDist) { minDist = d; nearest = i; }
            }

            uint16_t hue = _balls[nearest].hue;
            if (useColor) {
                hue = (uint16_t)(baseHue + nearest * 2400U + (uint16_t)(_balls[nearest].energy * 35.0f));
            }
            uint32_t c = ledMatrix.colorHSV(hue, trailSat, (uint8_t)(fade * 255));
            uint8_t r = (c >> 16) & 0xFF;
            uint8_t g = (c >> 8)  & 0xFF;
            uint8_t b =  c        & 0xFF;
            strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
        }
    }

    // Ballköpfe als helle Punkte obendrauf
    for (int i = 0; i < ballCount; i++) {
        int px = (int)(_balls[i].x + 0.5f);
        int py = (int)(_balls[i].y + 0.5f);
        if (px < 0 || px >= WIDTH || py < 0 || py >= HEIGHT) continue;
        uint16_t hue = _balls[i].hue;
        if (useColor) {
            hue = (uint16_t)(baseHue + i * 2400U + 700U);
        }
        uint32_t c = ledMatrix.colorHSV(hue, 255, 255);
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8)  & 0xFF;
        uint8_t b =  c        & 0xFF;
        strip->setPixelColor(XY(px, py), makeColorWithBrightness(r, g, b));
    }

    strip->show();
}
