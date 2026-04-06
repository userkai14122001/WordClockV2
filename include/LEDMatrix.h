#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LEDMatrix {
public:
    LEDMatrix(uint16_t pin, uint16_t width, uint16_t height);
    
    // Initialisierung
    void init();
    
    // Pixel-Operationen
    void setPixelXY(int x, int y, uint32_t color);
    void setPixelXY(int x, int y);  // Nutzt globale color-Variable
    void setPixel(uint16_t index, uint32_t color);
    void setPixel(uint16_t index);  // Nutzt globale color-Variable
    
    // Set pixel directly with RGB color
    void setPixelDirect(uint16_t index, uint32_t rgb) {
        if (index >= strip.numPixels()) return;
        uint8_t r = (rgb >> 16) & 0xFF;
        uint8_t g = (rgb >> 8) & 0xFF;
        uint8_t b = rgb & 0xFF;
        strip.setPixelColor(index, strip.Color(r, g, b));
    }
    void setPixelXYDirect(int x, int y, uint32_t rgb) {
        uint16_t index = xy(x, y);
        setPixelDirect(index, rgb);
    }
    
    // Matrix-Operationen
    void clear();
    void show();
    void fill(uint32_t color);
    
    // Farb-Konvertierung und -Effekte
    uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b);
    uint32_t colorHSV(uint16_t hue, uint8_t saturation, uint8_t value);
    uint32_t gamma32(uint32_t color);
    
    // Koordinaten-Mapping (Serpentine Layout)
    uint16_t xy(int x, int y) const;
    
    // Brightness-Verwaltung
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const;
    
    // Direct access to strip (if needed for legacy code)
    Adafruit_NeoPixel* getStrip() { return &strip; }
    const Adafruit_NeoPixel* getStrip() const { return &strip; }
    
    // Matrix-Dimensionen
    uint16_t getWidth() const { return width; }
    uint16_t getHeight() const { return height; }
    uint16_t getPixelCount() const { return width * height; }
    
private:
    Adafruit_NeoPixel strip;
    uint16_t width;
    uint16_t height;
    uint16_t pin;
};

#endif
