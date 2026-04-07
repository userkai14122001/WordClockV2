#include "effect_helpers.h"
#include "DebugManager.h"

void Fire2DEffect::update() {
    uint8_t fireSpeed = clampSpeed();
    uint8_t fireDensity = clampDensity();
    fireSpeed = (uint8_t)min(100, (int)fireSpeed + 10);
    uint16_t frameInterval = 80 - (uint32_t)(80 - 10) * (fireSpeed - 1) / 99;
    if (millis() - _last < frameInterval) return;
    _last = millis();

    // Lazy-Allocation des Heat-Buffers
    if (!_heat) {
        _heat = new uint8_t[HEIGHT * WIDTH];
        if (!_heat) {
            DebugManager::println(DebugCategory::Effects, "ERROR: Fire2D heat alloc failed");
            return;
        }
        memset(_heat, 0, HEIGHT * WIDTH);
    }

    const uint8_t intensity = clampIntensity();
    const float intensityNorm = (float)(intensity - 1) / 99.0f;
    const float densityNorm = (float)(fireDensity - 1) / 99.0f;

    // Intensity steuert die aktive Flammenhoehe: 1 -> 2 Reihen, 100 -> volle Matrix.
    int activeRows = 2 + (int)(((uint32_t)(HEIGHT - 2) * (uint32_t)(intensity - 1)) / 99U);
    if (intensity <= 10) {
        activeRows = min(3, HEIGHT);
    }
    activeRows = constrain(activeRows, 2, HEIGHT);
    const int flameTopY = HEIGHT - activeRows;

    const int wind = (int)random(-1, 2);
    const int coolBottom = (int)intensityMap(3, 1);
    const int coolTopExtra = (int)intensityMap(26, 8);

    // 1. Cooling: oberhalb der Flammenzone progressiv auskuehlen, unten nur leicht.
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int idx = y * WIDTH + x;
            int distAbove = flameTopY - y;
            if (distAbove > 0) {
                // Gentle cooling ramp to preserve irregular tongues above nominal height.
                int coolMin = 5 + distAbove * 4;
                int coolMax = 12 + distAbove * 8;
                _heat[idx] = (uint8_t)max(0, (int)_heat[idx] - (int)random(coolMin, coolMax));

                // Wider transition band with rare tongues to avoid any visible cutoff line.
                if (distAbove <= 4 && random(0, max(7, (int)intensityMap(18, 5))) == 0) {
                    int weakMin = (int)intensityMap(22, 78);
                    int weakMax = (int)intensityMap(52, 140);
                    _heat[idx] = (uint8_t)max((int)_heat[idx], (int)random(weakMin, weakMax + 1));
                }
                continue;
            }

            float rel = (activeRows > 1) ? ((float)(y - flameTopY) / (float)(activeRows - 1)) : 1.0f;
            int coolingBand = coolBottom + (int)((1.0f - rel) * (float)coolTopExtra);
            int cool = random(0, max(1, coolingBand));
            _heat[idx] = (uint8_t)max(0, (int)_heat[idx] - cool);
        }
    }

    // 2. Heat rises with extra transition rows to blur the flame ceiling.
    int riseStartY = max(0, flameTopY - 3);
    for (int y = riseStartY; y < HEIGHT - 2; y++) {
        float rel = (activeRows > 1) ? ((float)(y - flameTopY) / (float)(activeRows - 1)) : 1.0f;
        rel = constrain(rel, 0.0f, 1.0f);
        int flickerLossMax = (int)(6 + (1.0f - rel) * (float)intensityMap(14, 5));

        for (int x = 0; x < WIDTH; x++) {
            int srcX = x + wind;
            if (srcX < 0) srcX = 0;
            if (srcX >= WIDTH) srcX = WIDTH - 1;

            int b1 = _heat[(y + 1) * WIDTH + srcX];
            int b2 = _heat[(y + 2) * WIDTH + x];
            int bl = _heat[(y + 1) * WIDTH + max(0, srcX - 1)];
            int br = _heat[(y + 1) * WIDTH + min(WIDTH - 1, srcX + 1)];

            int raised = (b1 * 5 + b2 * 2 + bl + br) / 9;
            int flickerLoss = random(0, max(2, flickerLossMax));
            _heat[y * WIDTH + x] = (uint8_t)max(0, raised - flickerLoss);
        }
    }

    // 3. Re-ignite bottom bed: low intensity = kleine Glut, high = breite lodernde Basis.
    uint8_t sparkMin = (uint8_t)intensityMap(95, 210);
    uint8_t sparkMax = (uint8_t)intensityMap(150, 255);
    int sources = 1 + (int)(((uint32_t)(WIDTH + 2) * (uint32_t)(fireDensity - 1)) / 99U);
    int tongueWidthMax = 1 + (int)(((uint32_t)2 * (uint32_t)(intensity - 1)) / 99U); // 1..3

    for (int i = 0; i < sources; i++) {
        int x = random(0, WIDTH);
        int width = random(1, tongueWidthMax + 1);
        uint8_t spark = (uint8_t)random(sparkMin, sparkMax + 1);

        for (int k = 0; k < width; k++) {
            int xx = min(WIDTH - 1, x + k);
            int idxBottom = (HEIGHT - 1) * WIDTH + xx;
            _heat[idxBottom] = (uint8_t)max((int)_heat[idxBottom], (int)spark);

            int idxSecond = (HEIGHT - 2) * WIDTH + xx;
            int boost2 = (int)intensityMap(6, 42);
            _heat[idxSecond] = (uint8_t)min(255, max((int)_heat[idxSecond], (int)spark - boost2));

            if (activeRows >= 3) {
                int idxThird = (HEIGHT - 3) * WIDTH + xx;
                int boost3 = (int)intensityMap(30, 95);
                _heat[idxThird] = (uint8_t)min(255, max((int)_heat[idxThird], (int)spark - boost3));
            }
        }
    }

    if (random(0, max(2, (int)intensityMap(8, 2))) == 0) {
        _heat[(HEIGHT - 1) * WIDTH + random(0, WIDTH)] = 255;
    }

    // 3b. Spatial smoothing to avoid visible hard scanline steps.
    uint8_t filtered[HEIGHT * WIDTH];
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int self = _heat[y * WIDTH + x] * 4;
            int left = _heat[y * WIDTH + max(0, x - 1)];
            int right = _heat[y * WIDTH + min(WIDTH - 1, x + 1)];
            int up = _heat[max(0, y - 1) * WIDTH + x];
            int down = _heat[min(HEIGHT - 1, y + 1) * WIDTH + x];
            filtered[y * WIDTH + x] = (uint8_t)((self + left + right + up + down) / 8);
        }
    }

    // Blend filtered heat back for smoother gradients while preserving detail.
    for (int i = 0; i < HEIGHT * WIDTH; i++) {
        _heat[i] = (uint8_t)(((int)_heat[i] * 3 + (int)filtered[i] * 2) / 5);
    }

    // 4. Tiny sparks: very rare, small and short-lived.
    uint8_t emberHeatMin = (uint8_t)intensityMap(55, 95);
    uint8_t emberHeatMax = (uint8_t)intensityMap(80, 130);
    int spawnDivider = (int)densityMap(560, 150);
    int maxActiveEmbers = 1 + (int)(densityNorm * 3.0f);

    int activeEmbers = 0;
    for (int i = 0; i < EMBER_COUNT; i++) {
        if (_embers[i].active) activeEmbers++;
    }

    for (int i = 0; i < EMBER_COUNT; i++) {
        Ember& e = _embers[i];
        if (e.active) {
            e.y -= e.speed;
            e.heat = (uint8_t)max(0, (int)e.heat - (int)random((int)intensityMap(2, 4), (int)intensityMap(10, 6) + 1));
            float driftAmp = 0.10f + intensityNorm * 0.45f;
            e.x += ((float)random(-10, 11) * 0.1f) * driftAmp;
            if (e.y < (float)flameTopY - 1.0f || e.heat == 0 || e.x < -0.5f || e.x >= (float)WIDTH + 0.5f) {
                e.active = false;
            }
        } else if (activeEmbers < maxActiveEmbers && random(0, max(2, spawnDivider)) == 0) {
            e.x = (float)random(0, WIDTH);
            e.y = (float)(HEIGHT - 1 - random(0, min(2, activeRows - 1)));
            e.speed = 0.09f + intensityNorm * 0.30f + (float)random(0, 6) * 0.02f;
            e.heat = (uint8_t)random(emberHeatMin, emberHeatMax + 1);
            e.active = true;
            activeEmbers++;
        }
    }

    // 5. Build a non-periodic flame ceiling per column to avoid visible rolling waves.
    if (!_edgeInit) {
        for (int x = 0; x < WIDTH; x++) {
            _edgeOffset[x] = (float)random(-12, 13) * 0.05f;
            _edgeVel[x] = (float)random(-8, 9) * 0.01f;
        }
        _edgeInit = true;
    }

    for (int x = 0; x < WIDTH; x++) {
        int acc = 0;
        int samples = 0;
        for (int y = HEIGHT - 1; y >= max(flameTopY, HEIGHT - 3); y--) {
            acc += _heat[y * WIDTH + x];
            samples++;
        }
        float columnEnergy = (samples > 0) ? ((float)acc / (float)samples) / 255.0f : 0.0f;

        float left = _edgeOffset[max(0, x - 1)];
        float right = _edgeOffset[min(WIDTH - 1, x + 1)];
        float neighborAvg = (left + _edgeOffset[x] + right) / 3.0f;

        float randomTarget = ((float)random(-100, 101) / 100.0f) * (0.55f + intensityNorm * 1.65f);
        float liftByEnergy = -(columnEnergy * (0.35f + intensityNorm * 2.1f));
        float target = randomTarget + liftByEnergy;

        float turbulence = ((float)random(-100, 101) / 100.0f) * (0.05f + intensityNorm * 0.24f);
        _edgeVel[x] = _edgeVel[x] * 0.68f
                    + (target - _edgeOffset[x]) * 0.18f
                    + (neighborAvg - _edgeOffset[x]) * 0.14f
                    + turbulence;

        _edgeOffset[x] += _edgeVel[x];
        float maxRise = 0.9f + intensityNorm * 3.4f;
        float maxDrop = 1.0f + intensityNorm * 1.5f;
        _edgeOffset[x] = constrain(_edgeOffset[x], -maxRise, maxDrop);
    }

    if (random(0, max(3, (int)intensityMap(16, 6))) == 0) {
        int cx = random(0, WIDTH);
        float burst = ((float)random(20, 120) / 100.0f) * (0.35f + intensityNorm * 1.2f);
        _edgeVel[cx] -= burst;
        if (cx > 0) _edgeVel[cx - 1] -= burst * 0.45f;
        if (cx < WIDTH - 1) _edgeVel[cx + 1] -= burst * 0.45f;
    }

    // 6. Render heat -> unten Glut/Gold, oben roetlichere Flammenspitzen.
    for (int y = 0; y < HEIGHT; y++) {
        float fy = (HEIGHT > 1) ? ((float)y / (float)(HEIGHT - 1)) : 0.0f; // 0 top, 1 bottom
        float baseBoost = fy * fy;

        for (int x = 0; x < WIDTH; x++) {
            int h = _heat[y * WIDTH + x];

            float localFlameTopF = (float)flameTopY + _edgeOffset[x];

            // Soft mask around the dynamic flame edge removes hard cut lines.
            float edgeDist = (float)y - localFlameTopF;
            float softRange = 2.1f + intensityNorm * 1.9f;
            float mask = (edgeDist + softRange) / (2.0f * softRange);
            mask = constrain(mask, 0.0f, 1.0f);
            mask = mask * mask * (3.0f - 2.0f * mask); // smoothstep

            // Add local, non-periodic edge turbulence.
            if (edgeDist < 2.5f) {
                int grad = _heat[min(HEIGHT - 1, y + 1) * WIDTH + x] - _heat[max(0, y - 1) * WIDTH + x];
                h += grad / 3;
                h += random(-(int)intensityMap(3, 11), (int)intensityMap(4, 13));
            }

            h = (int)((float)h * mask);
            h = constrain(h, 0, 255);
            if (h < 4) {
                strip->setPixelColor(XY(x, y), 0);
                continue;
            }

            uint8_t r, g, b;
            // Palette: 0=Auto(Classic), 1=Warm, 2=Cool, 3=Natur, 4=Candy
            switch (effectPalette) {
                default:
                case 0: // Auto = Classic red/orange/gold
                case 1: // Warm = same
                    if (h < 40) {
                        r = (uint8_t)(h * 3); g = 0; b = 0;
                    } else if (h < 105) {
                        r = 145 + (uint8_t)((h - 40) * 110 / 65);
                        g = (uint8_t)((h - 40) * 20 / 65); b = 0;
                    } else if (h < 180) {
                        r = 255; g = 20 + (uint8_t)((h - 105) * 170 / 75);
                        b = (uint8_t)((h - 105) * 14 / 75);
                    } else {
                        r = 255; g = 190 + (uint8_t)((h - 180) * 65 / 75);
                        b = 12 + (uint8_t)((h - 180) * 24 / 75);
                    }
                    break;
                case 2: // Cool = blue/cyan/white
                    if (h < 60) {
                        r = 0; g = 0; b = (uint8_t)(h * 4);
                    } else if (h < 140) {
                        r = 0; g = (uint8_t)((h - 60) * 2);
                        b = (uint8_t)min(255, 240 + (h - 60));
                    } else if (h < 200) {
                        r = (uint8_t)((h - 140) * 3);
                        g = 160 + (uint8_t)((h - 140) * 95 / 60);
                        b = 255;
                    } else {
                        r = 180 + (uint8_t)((h - 200) * 75 / 55);
                        g = 230 + (uint8_t)((h - 200) * 25 / 55);
                        b = 255;
                    }
                    break;
                case 3: // Natur = green/lime/yellow
                    if (h < 50) {
                        r = 0; g = (uint8_t)(h * 3); b = 0;
                    } else if (h < 130) {
                        r = (uint8_t)((h - 50) * 2);
                        g = (uint8_t)min(255, 150 + (h - 50) * 1);
                        b = 0;
                    } else if (h < 200) {
                        r = 160 + (uint8_t)((h - 130) * 95 / 70);
                        g = 220 + (uint8_t)((h - 130) * 35 / 70);
                        b = 0;
                    } else {
                        r = 255; g = 255;
                        b = (uint8_t)((h - 200) * 2);
                    }
                    break;
                case 4: // Candy = pink/magenta/violet
                    if (h < 50) {
                        r = (uint8_t)(h * 3); g = 0; b = (uint8_t)(h * 2);
                    } else if (h < 130) {
                        r = 160 + (uint8_t)((h - 50) * 95 / 80);
                        g = 0; b = 100 + (uint8_t)((h - 50) * 155 / 80);
                    } else if (h < 200) {
                        r = 255; g = (uint8_t)((h - 130) * 120 / 70);
                        b = 255 - (uint8_t)((h - 130) * 80 / 70);
                    } else {
                        r = 255; g = 120 + (uint8_t)((h - 200) * 135 / 55);
                        b = 175 + (uint8_t)((h - 200) * 80 / 55);
                    }
                    break;
            }

            // Unten goldener Kern, oben roter und schlanker.
            r = (uint8_t)min(255, (int)r + (int)(baseBoost * 24.0f));
            g = (uint8_t)min(255, (int)g + (int)(baseBoost * 26.0f));
            float upperBlend = (activeRows > 1) ? ((float)(y - localFlameTopF) / (float)activeRows) : 1.0f;
            upperBlend = constrain(upperBlend, 0.0f, 1.0f);
            float gScale = 0.70f + 0.30f * upperBlend;
            float bScale = 0.42f + 0.58f * upperBlend;
            g = (uint8_t)(g * gScale);
            b = (uint8_t)(b * bScale);

            strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
        }
    }

    // Draw embers on top of flames.
    for (int i = 0; i < EMBER_COUNT; i++) {
        const Ember& e = _embers[i];
        if (!e.active) continue;
        int ex = (int)(e.x + 0.5f);
        int ey = (int)(e.y + 0.5f);
        if (ex >= 0 && ex < WIDTH && ey >= 0 && ey < HEIGHT) {
            strip->setPixelColor(XY(ex, ey),
                makeColorWithBrightness((uint8_t)(140 + e.heat), (uint8_t)(e.heat * 0.30f), 0));
        }
    }

    strip->show();
}
