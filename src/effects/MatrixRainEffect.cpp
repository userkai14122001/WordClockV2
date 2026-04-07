#include "effect_helpers.h"
#include "DebugManager.h"

void MatrixRainEffect::update() {
    // Trail-Buffer wird per Lazy-Allocation in update() allokiert (~90 Bytes bei Bedarf)
    if (!_cols) {
        _cols = new Column[WIDTH];
        if (!_cols) { DebugManager::println(DebugCategory::Effects, "ERROR: MatrixRain cols alloc failed"); return; }
        memset(_cols, 0, sizeof(Column) * WIDTH);
    }

    uint16_t frameInterval = speedToDelay(10, 100);
    if (millis() - _last < frameInterval) return;
    _last = millis();

    int targetActiveCols = (int)densityMap(1, WIDTH);
    if (targetActiveCols < 1) targetActiveCols = 1;
    uint8_t trailMin = 3;
    uint8_t trailMax = (uint8_t)intensityMap(4, HEIGHT);
    uint8_t pauseMax = (uint8_t)densityMap(28, 4);

    float colSpeed = speedMapF(0.25f, 1.05f);

    uint8_t headR = hasUserColor() ? colR(color) : 200;
    uint8_t headG = hasUserColor() ? colG(color) : 255;
    uint8_t headB = hasUserColor() ? colB(color) : 200;
    uint8_t trailFloor = (uint8_t)intensityMap(20, 70);

    int activeCols = 0;
    for (int x = 0; x < WIDTH; x++) {
        if (_cols[x].active) activeCols++;
    }

    clearMatrix();

    for (int x = 0; x < WIDTH; x++) {
        Column& c = _cols[x];

        if (!c.active) {
            if (c.timer > 0) { c.timer--; continue; }
            if (activeCols >= targetActiveCols) continue;
            c.head   = -1.0f;
            c.speed  = colSpeed * (0.7f + (float)random(0, 10) * 0.06f);
            c.len    = (uint8_t)random(trailMin, trailMax + 1);
            c.active = true;
            activeCols++;
        } else {
            c.head += c.speed;
            if ((int)c.head - (int)c.len > HEIGHT) {
                c.active = false;
                activeCols = max(0, activeCols - 1);
                c.timer  = (uint8_t)random(2, pauseMax);
            }
        }

        if (!c.active) continue;

        for (int t = 0; t < (int)c.len; t++) {
            int y = (int)c.head - t;
            if (y < 0 || y >= HEIGHT) continue;
            if (t == 0) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(headR, headG, headB));
            } else {
                float fade = 1.0f - (float)t / (float)c.len;
                uint8_t r = (uint8_t)max((float)trailFloor, headR * fade * fade * 0.90f);
                uint8_t g = (uint8_t)max((float)trailFloor, headG * fade * fade);
                uint8_t b = (uint8_t)max((float)(trailFloor / 3), headB * fade * 0.70f);
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
            }
        }
    }

    strip->show();
}
