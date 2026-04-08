#pragma once
#include <Arduino.h>
#include "matrix.h"
#include "config.h"

// ---------------------------------------------------------
// Brightness Gamma Lookup Table (2.2 gamma curve)
// For perceptually linear brightness control
// ---------------------------------------------------------
static constexpr uint8_t GAMMA_LUT[256] PROGMEM = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    3,  3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,
    7,  7,  7,  8,  8,  8,  9,  9,  9,  10, 10, 11, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20,
    21, 21, 22, 23, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30,
    31, 32, 32, 33, 34, 35, 35, 36, 37, 38, 38, 39, 40, 41, 42, 42,
    43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56,
    57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 73,
    74, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85, 86, 87, 88, 89, 91,
    92, 93, 94, 95, 97, 98, 99, 100,102,103,104,105,107,108,109,110,
    112,113,114,116,117,118,120,121,122,124,125,126,128,129,131,132,
    133,135,136,138,139,140,142,143,145,146,148,149,151,152,154,155,
    157,158,160,161,163,164,166,167,169,171,172,174,175,177,178,180,
    182,183,185,186,188,190,191,193,195,196,198,200,201,203,205,206,
    208,210,211,213,215,217,218,220,222,224,225,227,229,231,232,234
};

// Inline function: apply gamma correction to brightness value
static inline uint8_t applyGamma(uint8_t brightness) {
    return pgm_read_byte(&GAMMA_LUT[brightness]);
}

// ---------------------------------------------------------
// Word struct
// ---------------------------------------------------------
struct Word { int x, y, len; };

// ---------------------------------------------------------
// Shared renderer helpers
// ---------------------------------------------------------
uint32_t makeColorWithBrightness(uint8_t r, uint8_t g, uint8_t b);
void     drawWord(const Word& w);
void     showExtraMinutes(int minute);
void     showTime(int hour, int minute);
void     resetClockMorphState();
bool     setClockWordPosition(const String& key, const Word& w);
bool     getClockWordPosition(const String& key, Word& out);
void     resetClockWordPositionsToDefault();

// ---------------------------------------------------------
// Base Effect class
// ---------------------------------------------------------
class Effect {
public:
    virtual ~Effect() = default;
    virtual const char* name() const = 0;
    virtual void update() = 0;
    virtual void reset() {}
};

// ---------------------------------------------------------
// Startup animation (blocking, called once in setup)
// ---------------------------------------------------------
class StartupAnimation {
public:
    void run();
};

// ---------------------------------------------------------
// Concrete effects
// ---------------------------------------------------------
class WifiRingEffect : public Effect {
    int           _ring;
    int           _ringStep;
    uint32_t      _fixedColor;   // 0 = use global color variable
    int           _headPos[10];
    unsigned long _lastFrame;
public:
    WifiRingEffect(int ring = 0, uint32_t fixedColor = 0, int ringStep = 0)
        : _ring(ring), _ringStep(ringStep), _fixedColor(fixedColor), _headPos{}, _lastFrame(0) {}
    const char* name() const override { return "wifi"; }
    void update() override;
};

class WaterDropEffect : public Effect {
    float         _radius;
    unsigned long _lastFrame;
    bool          _singleMode;
public:
    explicit WaterDropEffect(bool singleMode = false)
        : _radius(-0.35f), _lastFrame(0), _singleMode(singleMode) {}
    const char* name() const override { return "waterdrop"; }
    void update() override;
    void reset() override { _radius = -0.35f; }
};

class LoveYouEffect : public Effect {
    unsigned long _lastFrame = 0;
    float         _phase     = 0.0f;
public:
    const char* name() const override { return "love"; }
    void update() override;
    void reset() override { _phase = 0.0f; }
};

class ColorloopEffect : public Effect {
    uint16_t _baseHue = 0;
public:
    const char* name() const override { return "colorloop"; }
    void update() override;
    void reset() override { _baseHue = 0; }
};

class ColorwipeEffect : public Effect {
    int      _pos = 0;
    uint16_t _hue = 0;
public:
    const char* name() const override { return "colorwipe"; }
    void update() override;
    void reset() override { _pos = 0; _hue = 0; }
};

class Fire2DEffect : public Effect {
    struct Ember { float x, y, speed; uint8_t heat; bool active; };
    static const int EMBER_COUNT = 5;
    uint8_t       *_heat;        // Lazy-allocated (110 Bytes)
    Ember         _embers[EMBER_COUNT];
    float         _edgeOffset[WIDTH];
    float         _edgeVel[WIDTH];
    bool          _edgeInit;
    unsigned long _last;
public:
    Fire2DEffect() : _heat(nullptr), _edgeInit(false), _last(0) {}
    ~Fire2DEffect() {
        if (_heat) { delete[] _heat; _heat = nullptr; }
    }
    const char* name() const override { return "fire2d"; }
    void update() override;
    void reset() override {
        if (_heat) memset(_heat, 0, HEIGHT * WIDTH);
        memset(_embers, 0, sizeof(_embers));
        _edgeInit = false;
    }
};

class MatrixRainEffect : public Effect {
    struct Column {
        float   head;
        float   speed;
        uint8_t len;
        bool    active;
        uint8_t timer;
    };
    Column        *_cols;         // Lazy-allocated (90 Bytes bei WIDTH=10)
    unsigned long _last;
public:
    MatrixRainEffect() : _cols(nullptr), _last(0) {}
    ~MatrixRainEffect() {
        if (_cols) { delete[] _cols; _cols = nullptr; }
    }
    const char* name() const override { return "matrix"; }
    void update() override;
    void reset() override { 
        if (_cols) memset(_cols, 0, sizeof(Column) * WIDTH); 
    }
};

class PlasmaEffect : public Effect {
    uint32_t      _t;
    unsigned long _last;
public:
    PlasmaEffect() : _t(0), _last(0) {}
    const char* name() const override { return "plasma"; }
    void update() override;
    void reset() override { _t = 0; }
};

// Wellen laufen von außen nach innen (Trichter-Effekt)
class InwardRippleEffect : public Effect {
    float         _radius;      // aktuelle Wellenposition (maxDist → 0)
    unsigned long _lastFrame;
public:
    InwardRippleEffect() : _radius(-1.0f), _lastFrame(0) {}
    const char* name() const override { return "inward"; }
    void update() override;
    void reset() override { _radius = -1.0f; }
};

// Pixel funkeln zufällig auf und ab – Sternenhimmel-Effekt
struct TwinkleStar {
    uint8_t x, y;
    uint8_t phase;    // 0..255 Sinuswert-Phase
    uint8_t speed;    // Phasenschritte pro Frame
    uint16_t hue;     // Farbton (HSV)
};

class TwinkleEffect : public Effect {
    static constexpr int MAX_STARS = 30;
    TwinkleStar   _stars[MAX_STARS];
    unsigned long _lastFrame;
public:
    TwinkleEffect() : _lastFrame(0) {}
    const char* name() const override { return "twinkle"; }
    void update() override;
    void reset() override {}
};

class GreenRingWaveEffect : public Effect {
    unsigned long _lastFrame;
    int _wavePos;
public:
    GreenRingWaveEffect() : _lastFrame(0), _wavePos(0) {}
    const char* name() const override { return "greenringwave"; }
    void update() override;
    void reset() override { _wavePos = 0; }
};

    // Bälle prallen mit Physik über die Matrix
    struct Ball {
        float x, y;       // Position (float für sub-pixel)
        float vx, vy;     // Geschwindigkeit
        uint16_t hue;     // Farbe
        float energy;     // 0..100, sinkt bei Bounce/Flugzeit
    };

    class BouncingBallsEffect : public Effect {
        static constexpr int MAX_BALLS = 8;
        Ball          _balls[MAX_BALLS];
        unsigned long _lastFrame;
        bool          _seeded;
        uint8_t       *_trail;      // Lazy-allocated bei erstem Update
    public:
        BouncingBallsEffect() : _lastFrame(0), _seeded(false), _trail(nullptr) {}
        ~BouncingBallsEffect() {
            if (_trail) { delete[] _trail; _trail = nullptr; }
        }
        const char* name() const override { return "balls"; }
        void update() override;
        void reset() override { _seeded = false; }
    };

    // Sanfte Polarlichter-Vorhänge
    class AuroraEffect : public Effect {
        float         _offset;
        unsigned long _lastFrame;
    public:
        AuroraEffect() : _offset(0.0f), _lastFrame(0) {}
        const char* name() const override { return "aurora"; }
        void update() override;
        void reset() override { _offset = 0.0f; }
    };;

    // Minecraft-Enchantment-Table Stil:
    // Partikel von außen in die Mitte mit leichtem Schweif
    struct EnchantmentParticle {
        float x, y;
        float vx, vy;
        float life;
        float maxLife;
        uint16_t hue;
        bool active;
    };

    class EnchantmentEffect : public Effect {
        static constexpr int MAX_PARTICLES = 32;
        EnchantmentParticle _p[MAX_PARTICLES];
        unsigned long _lastFrame;
        bool _seeded;
    public:
        EnchantmentEffect() : _p{}, _lastFrame(0), _seeded(false) {}
        const char* name() const override { return "enchantment"; }
        void update() override;
        void reset() override { _seeded = false; }
    };

    // Selbstwachsende Schlange
    struct SnakeSegment { uint8_t x, y; };

    class SnakeEffect : public Effect {
        static constexpr int MAX_LEN = WIDTH * HEIGHT;
        SnakeSegment  _body[MAX_LEN];
        int           _len;
        int8_t        _dx, _dy;
        uint8_t       _foodX, _foodY;
        unsigned long _lastFrame;
        bool          _seeded;
        uint16_t      _hue;
        // Highscore
        int           _highscore;
        bool          _isNewHighscore;
        // Death animation
        uint8_t       _deathStep;
        bool          _deathActive;
        unsigned long _deathLastFrame;
        // Highscore celebration
        uint8_t       _celebStep;
        bool          _celebActive;
        unsigned long _celebLastFrame;
    public:
        SnakeEffect() : _len(0), _dx(1), _dy(0), _foodX(0), _foodY(0),
                        _lastFrame(0), _seeded(false), _hue(0),
                        _highscore(0), _isNewHighscore(false),
                        _deathStep(0), _deathActive(false), _deathLastFrame(0),
                        _celebStep(0), _celebActive(false), _celebLastFrame(0) {}
        const char* name() const override { return "snake"; }
        void update() override;
        void reset() override { _seeded = false; _len = 0; _deathActive = false; _celebActive = false; }
    };
