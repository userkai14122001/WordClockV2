#include "effect_helpers.h"

// Words used by this effect
static const Word W_LOVE_FX = {3, 3, 4};
static const Word W_YOU_FX  = {4, 5, 3};
static bool sLoveDebugLogged = false;

void LoveYouEffect::update() {
    if (!sLoveDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] love active");
        sLoveDebugLogged = true;
    }

    clearMatrix();
    drawWord(W_LOVE_FX);
    drawWord(W_YOU_FX);
    strip->show();
}
