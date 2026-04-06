#include "effect_helpers.h"

static bool sEnchantmentDebugLogged = false;

static float frandRange(float a, float b) {
    return a + (b - a) * ((float)random(0, 10000) / 9999.0f);
}

static void spawnEnchantmentParticle(EnchantmentParticle& p, float cx, float cy) {
    const int side = random(0, 4);
    float x = 0.0f;
    float y = 0.0f;

    if (side == 0) {
        x = frandRange(-0.4f, WIDTH - 0.6f);
        y = -0.8f;
    } else if (side == 1) {
        x = WIDTH - 0.2f;
        y = frandRange(-0.4f, HEIGHT - 0.6f);
    } else if (side == 2) {
        x = frandRange(-0.4f, WIDTH - 0.6f);
        y = HEIGHT - 0.2f;
    } else {
        x = -0.8f;
        y = frandRange(-0.4f, HEIGHT - 0.6f);
    }

    float tx = cx + frandRange(-0.45f, 0.45f);
    float ty = cy + frandRange(-0.45f, 0.45f);
    float dx = tx - x;
    float dy = ty - y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.01f) len = 0.01f;

    float baseSpeed = frandRange(0.07f, 0.13f);
    p.x = x;
    p.y = y;
    p.vx = (dx / len) * baseSpeed + frandRange(-0.015f, 0.015f);
    p.vy = (dy / len) * baseSpeed + frandRange(-0.015f, 0.015f);
    p.life = 0.0f;
    p.maxLife = frandRange(12.0f, 30.0f);
    p.hue = (uint16_t)random(35000, 48000); // cyan -> violet range
    p.active = true;
}

static void addPixelSaturated(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;

    uint32_t prev = strip->getPixelColor(XY(x, y));
    uint8_t pr = (prev >> 16) & 0xFF;
    uint8_t pg = (prev >> 8) & 0xFF;
    uint8_t pb = prev & 0xFF;

    uint8_t nr = (uint8_t)min(255, (int)pr + (int)r);
    uint8_t ng = (uint8_t)min(255, (int)pg + (int)g);
    uint8_t nb = (uint8_t)min(255, (int)pb + (int)b);

    strip->setPixelColor(XY(x, y), makeColorWithBrightness(nr, ng, nb));
}

void EnchantmentEffect::update() {
    if (!sEnchantmentDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] enchantment active");
        sEnchantmentDebugLogged = true;
    }

    const uint16_t frameDelay = speedToDelay(16, 85);
    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    const float cx = (WIDTH - 1) / 2.0f;
    const float cy = (HEIGHT - 1) / 2.0f;

    const int targetParticles = 6 + (int)intensityMap(0, MAX_PARTICLES - 6);

    if (!_seeded) {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            _p[i].active = false;
        }
        _seeded = true;
    }

    clearMatrix();

    uint8_t baseR = colR(color), baseG = colG(color), baseB = colB(color);
    bool useColor = (color != 0);

    for (int i = 0; i < targetParticles; i++) {
        EnchantmentParticle& p = _p[i];
        if (!p.active) {
            spawnEnchantmentParticle(p, cx, cy);
        }

        float pullX = cx - p.x;
        float pullY = cy - p.y;
        p.vx += pullX * 0.0022f;
        p.vy += pullY * 0.0022f;

        float speedLimit = speedMapF(0.12f, 0.30f);
        float speed = sqrtf(p.vx * p.vx + p.vy * p.vy);
        if (speed > speedLimit) {
            float s = speedLimit / speed;
            p.vx *= s;
            p.vy *= s;
        }

        p.x += p.vx;
        p.y += p.vy;
        p.life += 1.0f;

        float distCenter = sqrtf((p.x - cx) * (p.x - cx) + (p.y - cy) * (p.y - cy));
        if (p.life >= p.maxLife || distCenter < 0.45f ||
            p.x < -1.2f || p.x > WIDTH + 0.2f || p.y < -1.2f || p.y > HEIGHT + 0.2f) {
            spawnEnchantmentParticle(p, cx, cy);
        }

        float lifeFade = 1.0f - (p.life / p.maxLife);
        if (lifeFade < 0.0f) lifeFade = 0.0f;
        float bright = 0.30f + lifeFade * 0.70f;

        uint8_t r, g, b;
        if (useColor) {
            r = (uint8_t)(baseR * bright);
            g = (uint8_t)(baseG * bright);
            b = (uint8_t)(baseB * bright);
        } else {
            uint32_t c = ledMatrix.colorHSV(p.hue, 220, (uint8_t)(bright * 255));
            r = (c >> 16) & 0xFF;
            g = (c >> 8) & 0xFF;
            b = c & 0xFF;
        }

        int px = (int)roundf(p.x);
        int py = (int)roundf(p.y);
        addPixelSaturated(px, py, r, g, b);
        addPixelSaturated(px - (int)(p.vx > 0 ? 1 : -1), py - (int)(p.vy > 0 ? 1 : -1),
                          (uint8_t)(r * 0.35f), (uint8_t)(g * 0.35f), (uint8_t)(b * 0.35f));
    }

    // Center glyph glow like an enchantment table hotspot
    {
        uint8_t cr = useColor ? baseR : 120;
        uint8_t cg = useColor ? baseG : 200;
        uint8_t cb = useColor ? baseB : 255;
        addPixelSaturated((int)roundf(cx), (int)roundf(cy),
                          (uint8_t)(cr * 0.45f), (uint8_t)(cg * 0.45f), (uint8_t)(cb * 0.45f));
    }

    strip->show();
}
