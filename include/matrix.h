#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LEDMatrix.h"

// Global LED Matrix instance
extern LEDMatrix ledMatrix;
extern Adafruit_NeoPixel* strip;

// Backward compatible wrapper functions
uint16_t XY(int x, int y);
void setPixelXY(int x, int y);
void setPixelXY(int x, int y, uint32_t color);
void clearMatrix();

// Direct access to LED functions
inline void showMatrix() { ledMatrix.show(); }
