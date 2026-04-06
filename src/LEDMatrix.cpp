#include "LEDMatrix.h"
#include "DebugManager.h"

// Externe Variablen aus main.cpp
extern uint32_t color;
extern uint8_t brightness;

LEDMatrix::LEDMatrix(uint16_t pin, uint16_t width, uint16_t height)
    : strip(width * height, pin, NEO_RGB + NEO_KHZ800),
      width(width), height(height), pin(pin) {
}

void LEDMatrix::init() {
    DebugManager::println(DebugCategory::LEDMatrix, "LEDMatrix: Initialisiere...");
    strip.begin();
    strip.show();
    DebugManager::printf(DebugCategory::LEDMatrix, "LEDMatrix: %dx%d Matrix (%d Pixel) bereit\n",
                         width, height, width * height);
}

uint16_t LEDMatrix::xy(int x, int y) const {
    if (x < 0 || x >= (int)width || y < 0 || y >= (int)height) {
        return 0;
    }
    
    // SERPENTINE LAYOUT - WordClock-Standard
    // Oberste Zeile beginnt rechts → links
    if (y % 2 == 0) {
        // Gerade Zeile: rechts → links
        return y * width + (width - 1 - x);
    } else {
        // Ungerade Zeile: links → rechts
        return y * width + x;
    }
}

void LEDMatrix::setPixel(uint16_t index, uint32_t color) {
    if (index >= width * height) return;
    // Convert RGB to GRB for Adafruit NeoPixel
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    strip.setPixelColor(index, strip.Color(r, g, b));
}

void LEDMatrix::setPixel(uint16_t index) {
    if (index >= width * height) return;
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    strip.setPixelColor(index, makeColor(r, g, b));
}

void LEDMatrix::setPixelXY(int x, int y, uint32_t color) {
    uint16_t index = xy(x, y);
    setPixel(index, color);
}

void LEDMatrix::setPixelXY(int x, int y) {
    uint16_t index = xy(x, y);
    setPixel(index);
}

void LEDMatrix::clear() {
    for (uint16_t i = 0; i < width * height; i++) {
        strip.setPixelColor(i, 0);
    }
}

void LEDMatrix::show() {
    strip.show();
}

void LEDMatrix::fill(uint32_t color) {
    // Convert RGB to GRB for Adafruit NeoPixel
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint32_t grb = strip.Color(r, g, b);
    for (uint16_t i = 0; i < width * height; i++) {
        strip.setPixelColor(i, grb);
    }
}

uint32_t LEDMatrix::makeColor(uint8_t r, uint8_t g, uint8_t b) {
    // Return standard RGB format 0xRRGGBB (internal format)
    // Conversion to GRB happens in setPixelColor()
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

uint32_t LEDMatrix::colorHSV(uint16_t hue, uint8_t saturation, uint8_t value) {
    // With NEO_RGB, ColorHSV already returns the format we need
    return strip.ColorHSV(hue, saturation, value);
}

uint32_t LEDMatrix::gamma32(uint32_t color) {
    return strip.gamma32(color);
}

void LEDMatrix::setBrightness(uint8_t brightness) {
    strip.setBrightness(brightness);
}

uint8_t LEDMatrix::getBrightness() const {
    return strip.getBrightness();
}
