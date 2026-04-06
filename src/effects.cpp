#include <Arduino.h>
#include "config.h"
#include "ControlConfig.h"
#include "matrix.h"
#include "effects.h"
#include "effects/effect_helpers.h"
#include "DebugManager.h"

// ---------------------------------------------------------
// WORD DEFINITIONS  (used by this file only)
// ---------------------------------------------------------
static const Word W_ES      = {0, 0, 2};
static const Word W_IST     = {3, 0, 3};
static const Word W_FUENF   = {7, 0, 4};
static const Word W_ZEHN    = {0, 1, 4};
static const Word W_ZWANZIG = {4, 1, 7};
static const Word W_VIERTEL = {4, 2, 7};
static const Word W_VOR     = {0, 3, 3};
static const Word W_NACH    = {7, 3, 4};
static const Word W_HALB    = {0, 4, 4};

static const Word H_ZWOELF  = {5, 8, 5};
static const Word H_EINS    = {0, 9, 4};
static const Word H_EIN     = {0, 9, 3};
static const Word H_ZWEI    = {7, 7, 4};
static const Word H_DREI    = {0, 5, 4};
static const Word H_VIER    = {7, 5, 4};
static const Word H_FUENF_H = {7, 4, 4};
static const Word H_SECHS   = {0, 6, 5};
static const Word H_SIEBEN  = {5, 6, 6};
static const Word H_ACHT    = {1, 8, 4};
static const Word H_NEUN    = {3, 7, 4};
static const Word H_ZEHN_H  = {0, 7, 4};
static const Word H_ELF     = {5, 4, 3};

static const Word W_UHR  = {4, 9, 3};
static const Word M1     = {7, 9, 1};
static const Word M2     = {8, 9, 1};
static const Word M3     = {9, 9, 1};
static const Word M4     = {10, 9, 1};
static const Word W_LOVE = {3, 3, 4};
static const Word W_YOU  = {4, 5, 3};

uint32_t makeColorWithBrightness(uint8_t r, uint8_t g, uint8_t b) {
    // Brightness is handled globally via ledMatrix.setBrightness().
    // Here we only apply color gamma for smoother low-level color rendering.
    return ledMatrix.gamma32(ledMatrix.makeColor(r, g, b));
}

void drawWord(const Word& w) {
    uint8_t r = colR(color);
    uint8_t g = colG(color);
    uint8_t b = colB(color);
    uint32_t pixelColor = ledMatrix.makeColor(r, g, b);
    pixelColor = ledMatrix.gamma32(pixelColor);
    for (int i = 0; i < w.len; i++)
        ledMatrix.setPixelXY(w.x + i, w.y, pixelColor);
}



// ---------------------------------------------------------
// EXTRA MINUTES
// ---------------------------------------------------------
void showExtraMinutes(int minute) {
    int e = minute % 5;
    if (e >= 1) drawWord(M1);
    if (e >= 2) drawWord(M2);
    if (e >= 3) drawWord(M3);
    if (e >= 4) drawWord(M4);
}

static void markWordMask(uint8_t* mask, const Word& w) {
    for (int i = 0; i < w.len; i++) {
        const int x = w.x + i;
        if (x < 0 || x >= WIDTH || w.y < 0 || w.y >= HEIGHT) {
            continue;
        }
        mask[w.y * WIDTH + x] = 1;
    }
}

static bool masksEqual(const uint8_t* left, const uint8_t* right) {
    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        if (left[i] != right[i]) {
            return false;
        }
    }
    return true;
}

static void buildTimeMask(uint8_t* mask, int hour, int minute) {
    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        mask[i] = 0;
    }

    markWordMask(mask, W_ES);
    markWordMask(mask, W_IST);

    int m = minute / 5;
    bool full = (minute < 5);
    switch (m) {
        case 1: markWordMask(mask, W_FUENF);   markWordMask(mask, W_NACH); break;
        case 2: markWordMask(mask, W_ZEHN);    markWordMask(mask, W_NACH); break;
        case 3: markWordMask(mask, W_VIERTEL); markWordMask(mask, W_NACH); break;
        case 4: markWordMask(mask, W_ZWANZIG); markWordMask(mask, W_NACH); break;
        case 5: markWordMask(mask, W_FUENF);   markWordMask(mask, W_VOR);  markWordMask(mask, W_HALB); hour++; break;
        case 6: markWordMask(mask, W_HALB);    hour++; break;
        case 7: markWordMask(mask, W_FUENF);   markWordMask(mask, W_NACH); markWordMask(mask, W_HALB); hour++; break;
        case 8: markWordMask(mask, W_ZWANZIG); markWordMask(mask, W_VOR);  hour++; break;
        case 9: markWordMask(mask, W_VIERTEL); markWordMask(mask, W_VOR);  hour++; break;
        case 10:markWordMask(mask, W_ZEHN);    markWordMask(mask, W_VOR);  hour++; break;
        case 11:markWordMask(mask, W_FUENF);   markWordMask(mask, W_VOR);  hour++; break;
    }

    hour %= 12;
    switch (hour) {
        case 0:  markWordMask(mask, H_ZWOELF); break;
        case 1:  full ? markWordMask(mask, H_EIN) : markWordMask(mask, H_EINS); break;
        case 2:  markWordMask(mask, H_ZWEI); break;
        case 3:  markWordMask(mask, H_DREI); break;
        case 4:  markWordMask(mask, H_VIER); break;
        case 5:  markWordMask(mask, H_FUENF_H); break;
        case 6:  markWordMask(mask, H_SECHS); break;
        case 7:  markWordMask(mask, H_SIEBEN); break;
        case 8:  markWordMask(mask, H_ACHT); break;
        case 9:  markWordMask(mask, H_NEUN); break;
        case 10: markWordMask(mask, H_ZEHN_H); break;
        case 11: markWordMask(mask, H_ELF); break;
    }

    if (full) {
        markWordMask(mask, W_UHR);
    }

    int e = minute % 5;
    if (e >= 1) markWordMask(mask, M1);
    if (e >= 2) markWordMask(mask, M2);
    if (e >= 3) markWordMask(mask, M3);
    if (e >= 4) markWordMask(mask, M4);
}

static void renderMaskFrame(const uint8_t* mask, uint8_t alpha255, uint8_t r, uint8_t g, uint8_t b) {
    const uint8_t rr = (uint16_t)r * alpha255 / 255;
    const uint8_t gg = (uint16_t)g * alpha255 / 255;
    const uint8_t bb = (uint16_t)b * alpha255 / 255;
    uint32_t pixelColor = ledMatrix.makeColor(rr, gg, bb);
    pixelColor = ledMatrix.gamma32(pixelColor);  // Apply gamma correction for proper dark color rendering

    clearMatrix();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mask[y * WIDTH + x]) {
                ledMatrix.setPixelXYDirect(x, y, pixelColor);
            }
        }
    }
    strip->show();  // Ensure display updates immediately
}

static uint8_t gClockPrevMask[LED_PIXEL_AMOUNT] = {0};
static bool gClockHasPrevMask = false;

void resetClockMorphState() {
    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        gClockPrevMask[i] = 0;
    }
    gClockHasPrevMask = false;
    DebugManager::println(DebugCategory::Effects, "[FX][clock] morph state reset");
}


// ---------------------------------------------------------
// TIME DISPLAY
// ---------------------------------------------------------
void showTime(int hour, int minute) {
    static int sLastLoggedMinute = -1;
    static uint16_t sLastLoggedMorphClampMs = 0;
    if (minute != sLastLoggedMinute) {
        sLastLoggedMinute = minute;
        DebugManager::printf(DebugCategory::Effects, "[FX][clock] showTime %02d:%02d\n", hour, minute);
    }

    uint8_t nextMask[LED_PIXEL_AMOUNT];
    buildTimeMask(nextMask, hour, minute);

    uint8_t r = colR(color);
    uint8_t g = colG(color);
    uint8_t b = colB(color);

    if (!gClockHasPrevMask) {
        renderMaskFrame(nextMask, 255, r, g, b);
        strip->show();
        for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
            gClockPrevMask[i] = nextMask[i];
        }
        gClockHasPrevMask = true;
        return;
    }

    if (masksEqual(gClockPrevMask, nextMask)) {
        renderMaskFrame(nextMask, 255, r, g, b);
        return;
    }

    const uint16_t morphDurationMs = min<uint16_t>(transitionMs, ControlConfig::CLOCK_MORPH_MAX_MS);
    if (transitionMs > ControlConfig::CLOCK_MORPH_MAX_MS) {
        if (sLastLoggedMorphClampMs != transitionMs) {
            sLastLoggedMorphClampMs = transitionMs;
            DebugManager::printf(DebugCategory::Effects,
                                 "[FX][clock] morph clamped %ums -> %ums\n",
                                 transitionMs,
                                 morphDurationMs);
        }
    } else {
        sLastLoggedMorphClampMs = 0;
    }

    // Use a bounded morph duration so MQTT/network work stays responsive.
    const uint8_t steps = 30;
    const uint16_t frameDelayMs = max((uint16_t)10, (uint16_t)(morphDurationMs / steps));

    for (uint8_t s = 1; s <= steps; s++) {
        uint8_t morphMask[LED_PIXEL_AMOUNT];
        float frac = (float)s / steps;
        float eased = frac * frac * (3.0f - 2.0f * frac);
        uint8_t t = (uint8_t)(eased * 255);

        for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
            if (gClockPrevMask[i] && nextMask[i]) {
                morphMask[i] = 255;
            } else if (!gClockPrevMask[i] && !nextMask[i]) {
                morphMask[i] = 0;
            } else if (!gClockPrevMask[i] && nextMask[i]) {
                morphMask[i] = t;
            } else {
                morphMask[i] = 255 - t;
            }
        }

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint8_t a = morphMask[y * WIDTH + x];
                if (!a) {
                    strip->setPixelColor(XY(x, y), 0);
                    continue;
                }
                uint8_t rr = (uint16_t)r * a / 255;
                uint8_t gg = (uint16_t)g * a / 255;
                uint8_t bb = (uint16_t)b * a / 255;
                uint32_t c = ledMatrix.makeColor(rr, gg, bb);
                ledMatrix.setPixelXYDirect(x, y, ledMatrix.gamma32(c));
            }
        }
        strip->show();
        waitMs(frameDelayMs);
    }

    renderMaskFrame(nextMask, 255, r, g, b);

    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        gClockPrevMask[i] = nextMask[i];
    }
}

