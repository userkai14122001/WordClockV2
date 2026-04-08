#include <Arduino.h>
#include "config.h"
#include "ControlConfig.h"
#include "matrix.h"
#include "effects.h"
#include "effects/effect_helpers.h"
#include "DebugManager.h"

// ---------------------------------------------------------
// WORD DEFINITIONS
// ---------------------------------------------------------
static Word W_ES      = {0, 0, 2};
static Word W_IST     = {3, 0, 3};
static Word W_FUENF   = {7, 0, 4};
static Word W_ZEHN    = {0, 1, 4};
static Word W_ZWANZIG = {4, 1, 7};
static Word W_VIERTEL = {4, 2, 7};
static Word W_VOR     = {0, 3, 3};
static Word W_NACH    = {7, 3, 4};
static Word W_HALB    = {0, 4, 4};

static Word H_ZWOELF  = {5, 8, 5};
static Word H_EINS    = {0, 9, 4};
static Word H_EIN     = {0, 9, 3};
static Word H_ZWEI    = {7, 7, 4};
static Word H_DREI    = {0, 5, 4};
static Word H_VIER    = {7, 5, 4};
static Word H_FUENF_H = {7, 4, 4};
static Word H_SECHS   = {0, 6, 5};
static Word H_SIEBEN  = {5, 6, 6};
static Word H_ACHT    = {1, 8, 4};
static Word H_NEUN    = {3, 7, 4};
static Word H_ZEHN_H  = {0, 7, 4};
static Word H_ELF     = {5, 4, 3};

static Word W_UHR  = {4, 9, 3};
static Word M1     = {7, 9, 1};
static Word M2     = {8, 9, 1};
static Word M3     = {9, 9, 1};
static Word M4     = {10, 9, 1};
static Word W_LOVE = {3, 3, 4};
static Word W_YOU  = {4, 5, 3};

void resetClockWordPositionsToDefault() {
    W_ES = {0, 0, 2};
    W_IST = {3, 0, 3};
    W_FUENF = {7, 0, 4};
    W_ZEHN = {0, 1, 4};
    W_ZWANZIG = {4, 1, 7};
    W_VIERTEL = {4, 2, 7};
    W_VOR = {0, 3, 3};
    W_NACH = {7, 3, 4};
    W_HALB = {0, 4, 4};

    H_ZWOELF = {5, 8, 5};
    H_EINS = {0, 9, 4};
    H_EIN = {0, 9, 3};
    H_ZWEI = {7, 7, 4};
    H_DREI = {0, 5, 4};
    H_VIER = {7, 5, 4};
    H_FUENF_H = {7, 4, 4};
    H_SECHS = {0, 6, 5};
    H_SIEBEN = {5, 6, 6};
    H_ACHT = {1, 8, 4};
    H_NEUN = {3, 7, 4};
    H_ZEHN_H = {0, 7, 4};
    H_ELF = {5, 4, 3};

    W_UHR = {4, 9, 3};
    M1 = {7, 9, 1};
    M2 = {8, 9, 1};
    M3 = {9, 9, 1};
    M4 = {10, 9, 1};
    W_LOVE = {3, 3, 4};
    W_YOU = {4, 5, 3};
}

bool setClockWordPosition(const String& key, const Word& w) {
    if (key == "ES") W_ES = w;
    else if (key == "IST") W_IST = w;
    else if (key == "FUENF") W_FUENF = w;
    else if (key == "ZEHN") W_ZEHN = w;
    else if (key == "ZWANZIG") W_ZWANZIG = w;
    else if (key == "VIERTEL") W_VIERTEL = w;
    else if (key == "VOR") W_VOR = w;
    else if (key == "NACH") W_NACH = w;
    else if (key == "HALB") W_HALB = w;
    else if (key == "ZWOELF") H_ZWOELF = w;
    else if (key == "EINS") H_EINS = w;
    else if (key == "EIN") H_EIN = w;
    else if (key == "ZWEI") H_ZWEI = w;
    else if (key == "DREI") H_DREI = w;
    else if (key == "VIER") H_VIER = w;
    else if (key == "FUENF_H") H_FUENF_H = w;
    else if (key == "SECHS") H_SECHS = w;
    else if (key == "SIEBEN") H_SIEBEN = w;
    else if (key == "ACHT") H_ACHT = w;
    else if (key == "NEUN") H_NEUN = w;
    else if (key == "ZEHN_H") H_ZEHN_H = w;
    else if (key == "ELF") H_ELF = w;
    else if (key == "UHR") W_UHR = w;
    else if (key == "M1") M1 = w;
    else if (key == "M2") M2 = w;
    else if (key == "M3") M3 = w;
    else if (key == "M4") M4 = w;
    else if (key == "LOVE") W_LOVE = w;
    else if (key == "YOU") W_YOU = w;
    else return false;
    return true;
}

bool getClockWordPosition(const String& key, Word& out) {
    if (key == "ES") out = W_ES;
    else if (key == "IST") out = W_IST;
    else if (key == "FUENF") out = W_FUENF;
    else if (key == "ZEHN") out = W_ZEHN;
    else if (key == "ZWANZIG") out = W_ZWANZIG;
    else if (key == "VIERTEL") out = W_VIERTEL;
    else if (key == "VOR") out = W_VOR;
    else if (key == "NACH") out = W_NACH;
    else if (key == "HALB") out = W_HALB;
    else if (key == "ZWOELF") out = H_ZWOELF;
    else if (key == "EINS") out = H_EINS;
    else if (key == "EIN") out = H_EIN;
    else if (key == "ZWEI") out = H_ZWEI;
    else if (key == "DREI") out = H_DREI;
    else if (key == "VIER") out = H_VIER;
    else if (key == "FUENF_H") out = H_FUENF_H;
    else if (key == "SECHS") out = H_SECHS;
    else if (key == "SIEBEN") out = H_SIEBEN;
    else if (key == "ACHT") out = H_ACHT;
    else if (key == "NEUN") out = H_NEUN;
    else if (key == "ZEHN_H") out = H_ZEHN_H;
    else if (key == "ELF") out = H_ELF;
    else if (key == "UHR") out = W_UHR;
    else if (key == "M1") out = M1;
    else if (key == "M2") out = M2;
    else if (key == "M3") out = M3;
    else if (key == "M4") out = M4;
    else if (key == "LOVE") out = W_LOVE;
    else if (key == "YOU") out = W_YOU;
    else return false;
    return true;
}

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

