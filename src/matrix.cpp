#include "matrix.h"
#include "DebugManager.h"

// Externe Variablen aus main.cpp
extern uint32_t color;
Adafruit_NeoPixel* strip = nullptr;

// ---------------------------------------------------------
// Backward-compatible wrapper functions using LEDMatrix
// ---------------------------------------------------------

uint16_t XY(int x, int y) {
    return ledMatrix.xy(x, y);
}

void setPixelXY(int x, int y) {
    ledMatrix.setPixelXY(x, y);
}

void setPixelXY(int x, int y, uint32_t color) {
    ledMatrix.setPixelXY(x, y, color);
}

void clearMatrix() {
    static bool sMatrixDebugLogged = false;
    if (!sMatrixDebugLogged) {
        DebugManager::println(DebugCategory::LEDMatrix, "matrix.cpp wrapper active");
        sMatrixDebugLogged = true;
    }
    ledMatrix.clear();
}
