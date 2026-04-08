#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include "ota_https_update.h"

#include "WiFiManager.h"
#include "MQTTManager.h"
#include "RTCManager.h"
#include "LEDMatrix.h"
#include "StateManager.h"
#include "EffectManager.h"
#include "TimeManager.h"
#include "MemoryManager.h"
#include "SerialCommands.h"
#include "web_pages.h"
#include "matrix.h"
#include "effects.h"
#include "config.h"
#include "ControlConfig.h"
#include "DebugManager.h"

struct ControlUpdate;

// Forward Declarations
void handleCurrentEffect();
void runFullTimeTest();
void runSmokeTest();
void runSelfTest();
void transitionToEffect(const String& newEffect);
static void enqueuePendingMqttControl(const ControlUpdate& update);
static void processPendingMqttControl();
static void processPendingMqttColor();
static void syncRtcFromNtpIfNeeded();
static void logBootSection(const __FlashStringHelper* title);
static void configureMqttCallbacks();
static void updateBootSequence();
static void renderAfterControlChange(bool changed, bool effectChanged, bool colorAppliedNow);
static void publishAfterControlChange(bool changed, bool publishState);
void applyControlUpdate(
    bool hasPower, bool newPower,
    bool hasBrightness, uint8_t newBrightness,
    bool hasColor, uint32_t newColor,
    bool hasEffect, const String& newEffect,
    bool debounceColor,
    bool renderNow,
    bool publishState
);

struct ControlUpdate {
    bool hasPower = false;
    bool newPower = true;

    bool hasBrightness = false;
    uint8_t newBrightness = 120;

    bool hasColor = false;
    uint32_t newColor = 0;

    bool hasEffect = false;
    String newEffect;
};

static ControlUpdate parseMqttControlUpdate(const DynamicJsonDocument& doc);

struct PendingMqttControl {
    bool pending = false;
    ControlUpdate update;
    unsigned long queuedAtMs = 0;
    uint32_t sequence = 0;
};

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

static void logBootSection(const __FlashStringHelper* title) {
    DebugManager::println(DebugCategory::Boot);
    DebugManager::println(DebugCategory::Boot, F("--------------------------------------------------"));
    DebugManager::print(DebugCategory::Boot, F("[BOOT] "));
    DebugManager::println(DebugCategory::Boot, title);
    DebugManager::println(DebugCategory::Boot, F("--------------------------------------------------"));
}

// ---------------------------------------------------------
// Globale Objekte (Manager-Klassen)
// ---------------------------------------------------------
WiFiManager wifiManager;
WiFiClient espClient;
MQTTManager mqttManager(espClient, "wordclock");
RTCManager rtcManager(GPIO_NUM_6, GPIO_NUM_7);
LEDMatrix ledMatrix(LED_PIN, WIDTH, HEIGHT);

// Singleton Manager References
StateManager& stateManager = StateManager::getInstance();
EffectManager& effectManager = EffectManager::getInstance();
TimeManager& timeManager = TimeManager::getInstance();

// WordClock Variablen (for backward compatibility)
bool powerState      = true;
String currentEffect = "clock";
uint32_t color       = ControlConfig::DEFAULT_COLOR;
uint8_t brightness   = ControlConfig::DEFAULT_BRIGHTNESS;

// Effect tuning parameters (accessible from effects.cpp via extern)
uint8_t  effectSpeed     = ControlConfig::DEFAULT_SPEED;
uint8_t  effectIntensity = ControlConfig::DEFAULT_INTENSITY;
uint8_t  effectDensity   = ControlConfig::DEFAULT_DENSITY;
uint16_t transitionMs    = ControlConfig::DEFAULT_TRANSITION_MS;
uint8_t  effectPalette   = ControlConfig::DEFAULT_PALETTE;
uint16_t effectHueShift  = ControlConfig::DEFAULT_HUE_SHIFT;

// Frame buffer: Speichert den aktuellen Zustand aller 110 LEDs
// Wird regelmäßig aktualisiert und garantiert, dass ALLE LEDs in jedem Frame
// mit ihrem bekannten Zustand aktualisiert werden (verhindert Ghosting)
static uint32_t gFrameBuffer[LED_PIXEL_AMOUNT] = {0};

// Debugging: Protokolliere wie oft wir den Frame-Buffer aktualisieren
static unsigned long gLastFrameBufferUpdateMs = 0;
static uint16_t gFrameBufferUpdateCount = 0;

// Debounce für MQTT state publish: verhindert Feedbackloop mit HA
MQTTManager* g_mqttManager = nullptr;

// Debounce incoming HA color spam: apply only the last color after a short settle time.
static bool gHasPendingMqttColor = false;
static uint32_t gPendingMqttColor = 0;
static unsigned long gPendingMqttColorAt = 0;
static unsigned long gLastAppliedMqttColorAt = 0;
static const uint16_t MQTT_COLOR_SETTLE_MS = ControlConfig::MQTT_COLOR_SETTLE_MS;
static const uint16_t MQTT_COLOR_MIN_APPLY_INTERVAL_MS = ControlConfig::MQTT_COLOR_MIN_APPLY_INTERVAL_MS;
static const uint16_t MQTT_CONTROL_SETTLE_MS = 60;
static PendingMqttControl gPendingMqttControl;
static uint32_t gNextMqttControlSequence = 1;
static unsigned long gLastLoopDurationWarnAt = 0;
static unsigned long gLastLoopHeartbeatMs = 0;
static uint32_t gLastEffectUpdateUs = 0;
static uint32_t gMaxEffectUpdateUsWindow = 0;
static uint32_t gEffectUpdateSamples = 0;

static constexpr unsigned long MQTT_QUEUE_WARN_MS = 75;
static constexpr uint32_t MQTT_APPLY_WARN_US = 40000;
static constexpr uint32_t MQTT_PUBLISH_WARN_US = 30000;
static constexpr unsigned long MAIN_LOOP_WARN_MS = 50;
static constexpr uint32_t EFFECT_UPDATE_WARN_US = 12000;
static constexpr unsigned long LOOP_HEARTBEAT_MS = 2000;
static constexpr unsigned long OTA_AUTO_CHECK_INTERVAL_MS = 60UL * 60UL * 1000UL; // 1 h
static constexpr unsigned long OTA_FIRST_CHECK_DELAY_MS = 30UL * 1000UL;           // 30 s after boot

// ---------------------------------------------------------
// Effect instances
// ---------------------------------------------------------
WifiRingEffect  fxWifi;                        // "wifi" effect (ring=0, global color)
WifiRingEffect  fxSetupWifi(0, 0x0000FF);      // Setup-Mode animation (nur aeusserer Ring, blau)
WaterDropEffect fxWaterDrop;
LoveYouEffect   fxLoveYou;
ColorloopEffect fxColorloop;
ColorwipeEffect fxColorwipe;
Fire2DEffect    fxFire2D;
MatrixRainEffect fxMatrix;
PlasmaEffect    fxPlasma;
InwardRippleEffect fxInward;
TwinkleEffect      fxTwinkle;
BouncingBallsEffect fxBalls;
AuroraEffect        fxAurora;
EnchantmentEffect   fxEnchantment;
SnakeEffect         fxSnake;


Effect* const allEffects[] = {
    &fxWifi, &fxWaterDrop, &fxLoveYou,
    &fxColorloop, &fxColorwipe, &fxFire2D, &fxMatrix, &fxPlasma, &fxInward, &fxTwinkle,
    &fxBalls, &fxAurora, &fxEnchantment, &fxSnake
};

enum class BootPhase {
    StartupSweep,
    StartupHold,
    StartupFade,
    WifiAnim,
    WifiConnect,
    NetInit,
    MqttAnim,
    MqttConnect,
    MqttFallbackDots,
    Done
};

static BootPhase gBootPhase = BootPhase::StartupSweep;
static bool gBootActive = true;
static unsigned long gBootPhaseStartMs = 0;
static unsigned long gBootLastStepMs = 0;
static int gBootStartupColumn = 0;
static int gBootStartupFadeStep = 250;
static uint16_t gBootWifiFrames = 0;
static uint16_t gBootWifiTargetFrames = 0;
static uint16_t gBootMqttFrames = 0;
static uint8_t gBootMinuteDotIndex = 0;
static bool gBootMqttConfigured = false;
static bool gBootSuppressVisuals = false;
static WifiRingEffect gBootMqttRingFx(1, 0xFF9900);

static uint16_t bootWifiAnimOnePassFrames() {
    // One full loop on the outer ring (ring 0).
    return (uint16_t)max(1, 2 * (WIDTH + HEIGHT) - 4);
}

static void renderEffectNow(const String& effectName) {
    if (effectName == "clock") {
        struct tm t;
        if (getLocalTime(&t)) {
            showTime(t.tm_hour, t.tm_min);
            return;
        }
        if (rtcManager.isAvailable()) {
            DateTime rtcNow = rtcManager.getTime();
            showTime(rtcNow.hour(), rtcNow.minute());
            return;
        }
        showTime(12, 0);
        return;
    }

    Effect* fx = effectManager.getEffect(effectName);
    if (fx) {
        fx->reset();
        fx->update();
    }
}

static void fadeDisplayBrightness(uint8_t fromBrightness, uint8_t toBrightness, uint8_t steps, uint16_t frameDelayMs) {
    if (!strip) return;
    if (steps == 0) {
        ledMatrix.setBrightness(toBrightness);
        strip->show();
        return;
    }

    int from = fromBrightness;
    int to = toBrightness;
    for (uint8_t i = 0; i <= steps; i++) {
        uint8_t b = (uint8_t)(from + ((to - from) * i) / steps);
        ledMatrix.setBrightness(b);
        strip->show();
        waitMs(frameDelayMs);
    }
}

static inline uint32_t blendPackedColor(uint32_t fromColor, uint32_t toColor, uint8_t alpha255) {
    uint8_t f0 = (uint8_t)(fromColor & 0xFF);
    uint8_t f1 = (uint8_t)((fromColor >> 8) & 0xFF);
    uint8_t f2 = (uint8_t)((fromColor >> 16) & 0xFF);
    uint8_t t0 = (uint8_t)(toColor & 0xFF);
    uint8_t t1 = (uint8_t)((toColor >> 8) & 0xFF);
    uint8_t t2 = (uint8_t)((toColor >> 16) & 0xFF);

    uint8_t b0 = (uint8_t)(f0 + ((int16_t)(t0 - f0) * alpha255) / 255);
    uint8_t b1 = (uint8_t)(f1 + ((int16_t)(t1 - f1) * alpha255) / 255);
    uint8_t b2 = (uint8_t)(f2 + ((int16_t)(t2 - f2) * alpha255) / 255);

    return ((uint32_t)b2 << 16) | ((uint32_t)b1 << 8) | b0;
}

static void captureFrame(uint32_t* frameOut) {
    if (!strip || frameOut == nullptr) return;
    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        frameOut[i] = strip->getPixelColor(i);
    }
}

// Sync frame buffer from strip and ensure all pixels are up-to-date
static inline void syncFrameBuffer() {
    captureFrame(gFrameBuffer);
}

// Flush entire frame buffer to all LEDs and show - prevents LED ghosting
// by ensuring EVERY pixel gets updated in every cycle
static inline void flushFrameBufferFast() {
    if (!strip) return;
    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        strip->setPixelColor(i, gFrameBuffer[i]);
    }
    strip->show();
}

static void showBlendedFrame(const uint32_t* fromFrame, const uint32_t* toFrame, uint8_t alpha255) {
    if (!strip || fromFrame == nullptr || toFrame == nullptr) return;
    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        strip->setPixelColor(i, blendPackedColor(fromFrame[i], toFrame[i], alpha255));
    }
    strip->show();
}

void transitionToEffect(const String& newEffect) {
    if (newEffect.isEmpty() || newEffect == currentEffect) {
        return;
    }

    const String previousEffect = currentEffect;
    const uint8_t targetBrightness = brightness;
    uint32_t fromFrame[LED_PIXEL_AMOUNT];
    uint32_t toFrame[LED_PIXEL_AMOUNT];

    captureFrame(fromFrame);

    if (previousEffect == "clock" || newEffect == "clock") {
        resetClockMorphState();
    }

    currentEffect = newEffect;
    stateManager.setCurrentEffect(newEffect);
    if (newEffect != "clock") {
        effectManager.setEffect(newEffect);
    }

    // Render first target frame, then crossfade from current framebuffer.
    renderEffectNow(newEffect);
    captureFrame(toFrame);

    const uint8_t steps = 14;
    const uint16_t frameDelayMs = 16;
    ledMatrix.setBrightness(targetBrightness);
    for (uint8_t s = 1; s <= steps; s++) {
        float frac = (float)s / (float)steps;
        float eased = frac * frac * (3.0f - 2.0f * frac);
        uint8_t a = (uint8_t)(eased * 255.0f);
        showBlendedFrame(fromFrame, toFrame, a);
        waitMs(frameDelayMs);
    }

    for (int i = 0; i < LED_PIXEL_AMOUNT; i++) {
        strip->setPixelColor(i, toFrame[i]);
    }
    strip->show();
    ledMatrix.setBrightness(targetBrightness);
}

static void renderAfterControlChange(bool changed, bool effectChanged, bool colorAppliedNow) {
    if (!powerState) {
        clearMatrix();
        strip->show();
        return;
    }

    if (effectChanged) {
        // transitionToEffect rendered the first frame already.
        return;
    }

    if (colorAppliedNow && currentEffect == "clock") {
        // Color-only change on clock should not trigger long morph.
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            resetClockMorphState();
            showTime(timeinfo.tm_hour, timeinfo.tm_min);
        }
        return;
    }

    if (!changed) {
        return;
    }

    if (currentEffect == "clock") {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            showTime(timeinfo.tm_hour, timeinfo.tm_min);
        }
    } else {
        handleCurrentEffect();
    }
}

static void publishAfterControlChange(bool changed, bool publishState) {
    if (publishState && changed && g_mqttManager) {
        g_mqttManager->publishState(powerState, currentEffect, color, brightness);
    }
}

static ControlUpdate parseMqttControlUpdate(const DynamicJsonDocument& doc) {
    ControlUpdate update;

    update.hasPower = doc.containsKey("state");
    update.newPower = update.hasPower ? (String(doc["state"].as<const char*>()) == "ON") : powerState;

    update.hasBrightness = doc.containsKey("brightness");
    update.newBrightness = update.hasBrightness ? (uint8_t)doc["brightness"].as<int>() : brightness;

    update.hasColor = doc.containsKey("color");
    update.newColor = color;
    if (update.hasColor) {
        int r = doc["color"]["r"].as<int>();
        int g = doc["color"]["g"].as<int>();
        int b = doc["color"]["b"].as<int>();
        update.newColor = (uint32_t)((r << 16) | (g << 8) | b);
    }

    update.hasEffect = doc.containsKey("effect");
    update.newEffect = update.hasEffect ? String(doc["effect"].as<const char*>()) : "";

    return update;
}

void applyControlUpdate(
    bool hasPower, bool newPower,
    bool hasBrightness, uint8_t newBrightness,
    bool hasColor, uint32_t newColor,
    bool hasEffect, const String& newEffect,
    bool debounceColor,
    bool renderNow,
    bool publishState
) {
    bool changed = false;
    bool effectChanged = false;
    bool colorAppliedNow = false;
    const bool deferPublishUntilColorApplied = debounceColor && hasColor && (!hasPower || newPower);

    if (hasPower && newPower != powerState) {
        stateManager.setPowerState(newPower);
        powerState = newPower;
        changed = true;
    }

    if (hasBrightness && newBrightness != brightness) {
        stateManager.setBrightness(newBrightness);
        brightness = newBrightness;
        ledMatrix.setBrightness(newBrightness);
        changed = true;
    }

    if (hasColor && newColor != color) {
        if (debounceColor) {
            gPendingMqttColor = newColor;
            gPendingMqttColorAt = millis();
            gHasPendingMqttColor = true;
        } else {
            stateManager.setColor(newColor);
            color = newColor;
            gLastAppliedMqttColorAt = millis();
            changed = true;
            colorAppliedNow = true;
        }
    }

    if (hasEffect && !newEffect.isEmpty()) {
        if (newEffect == "clock" || effectManager.getEffect(newEffect) != nullptr) {
            if (newEffect != currentEffect) {
                transitionToEffect(newEffect);
                effectChanged = true;
                changed = true;
            }
        } else {
            DebugManager::print(DebugCategory::Main, "Ungueltiger Effekt ignoriert: ");
            DebugManager::println(DebugCategory::Main, newEffect);
        }
    }

    if (renderNow) {
        renderAfterControlChange(changed, effectChanged, colorAppliedNow);
    }

    publishAfterControlChange(changed, publishState && !deferPublishUntilColorApplied);
}

// ---------------------------------------------------------
// SETUP
// ---------------------------------------------------------
void setup() {
    Serial.begin(115200);
    // Native USB-CDC: erst warten bis Host-Monitor verbunden, DANN drucken
    // 8s Timeout damit nach Reset genug Zeit zum Reconnect bleibt
    unsigned long t0 = millis();
    while (!Serial && (millis() - t0) < 8000) waitMs(10);
    waitMs(200); // USB-Stack des Host stabilisieren lassen

    DebugManager::println(DebugCategory::Boot);
    DebugManager::println(DebugCategory::Boot, "============================================");
    DebugManager::println(DebugCategory::Boot, "   WordClock - ESP32-C3  booting...");
    DebugManager::println(DebugCategory::Boot, "   Firmware: " __DATE__ " " __TIME__);
    DebugManager::println(DebugCategory::Boot, "============================================");

    logBootSection(F("Storage"));
    if (!SPIFFS.begin(true)) {
        DebugManager::println(DebugCategory::Boot, "SPIFFS: Mount fehlgeschlagen");
    } else {
        DebugManager::println(DebugCategory::Boot, "SPIFFS: Bereit");
    }

    logBootSection(F("RTC"));
    rtcManager.init();

    logBootSection(F("LED & Manager Init"));
    ledMatrix.init();
    strip = ledMatrix.getStrip();
    ledMatrix.setBrightness(brightness);

    // Initialize Managers
    timeManager.init(rtcManager);

    // Register effects
    for (Effect* e : allEffects) {
        effectManager.registerEffect(e->name(), e);
    }
    effectManager.setEffect("clock");

    // Load and apply persisted state
    stateManager.loadFromPreferences();
    powerState = stateManager.getPowerState();
    currentEffect = stateManager.getCurrentEffect();
    color = stateManager.getColor();
    brightness = stateManager.getBrightness();
    effectSpeed     = stateManager.getSpeed();
    effectIntensity = stateManager.getIntensity();
    effectDensity   = stateManager.getDensity();
    transitionMs    = stateManager.getTransitionMs();
    effectPalette   = stateManager.getPalette();
    effectHueShift  = stateManager.getHueShift();

    bool tuningCorrected = false;
    if (effectSpeed < ControlConfig::SPEED_MIN || effectSpeed > ControlConfig::SPEED_MAX) {
        effectSpeed = ControlConfig::DEFAULT_SPEED;
        stateManager.setSpeed(effectSpeed);
        tuningCorrected = true;
    }
    if (effectIntensity < ControlConfig::INTENSITY_MIN || effectIntensity > ControlConfig::INTENSITY_MAX) {
        effectIntensity = ControlConfig::DEFAULT_INTENSITY;
        stateManager.setIntensity(effectIntensity);
        tuningCorrected = true;
    }
    if (effectDensity < ControlConfig::DENSITY_MIN || effectDensity > ControlConfig::DENSITY_MAX) {
        effectDensity = ControlConfig::DEFAULT_DENSITY;
        stateManager.setDensity(effectDensity);
        tuningCorrected = true;
    }
    if (transitionMs < ControlConfig::TRANSITION_MIN_MS || transitionMs > ControlConfig::TRANSITION_MAX_MS) {
        transitionMs = ControlConfig::DEFAULT_TRANSITION_MS;
        stateManager.setTransitionMs(transitionMs);
        tuningCorrected = true;
    }
    if (tuningCorrected) {
        stateManager.saveToPreferences();
    }

    bool bootStateCorrected = false;
    if (brightness < ControlConfig::BRIGHTNESS_MIN || brightness > ControlConfig::BRIGHTNESS_MAX) {
        DebugManager::print(DebugCategory::Boot, "Persisted brightness invalid, fallback to default (was ");
        DebugManager::print(DebugCategory::Boot, brightness);
        DebugManager::println(DebugCategory::Boot, ")");
        brightness = ControlConfig::DEFAULT_BRIGHTNESS;
        stateManager.setBrightness(brightness);
        bootStateCorrected = true;
    }

    if (currentEffect != "clock" && effectManager.getEffect(currentEffect) == nullptr) {
        DebugManager::print(DebugCategory::Boot, "Persisted effect invalid, fallback to clock: ");
        DebugManager::println(DebugCategory::Boot, currentEffect);
        currentEffect = "clock";
        stateManager.setCurrentEffect(currentEffect);
        bootStateCorrected = true;
    }

    if (bootStateCorrected) {
        stateManager.saveToPreferences();
    }

    ledMatrix.setBrightness(brightness);

    effectManager.setEffect(currentEffect);
    clearMatrix();
    strip->show();

    gBootActive = true;
    gBootSuppressVisuals = !powerState;
    gBootPhase = gBootSuppressVisuals ? BootPhase::WifiConnect : BootPhase::StartupSweep;
    gBootPhaseStartMs = millis();
    gBootLastStepMs = 0;
    gBootStartupColumn = 0;
    gBootStartupFadeStep = 250;
    gBootWifiFrames = 0;
    gBootWifiTargetFrames = bootWifiAnimOnePassFrames();
    gBootMqttFrames = 0;
    gBootMinuteDotIndex = 0;

    logBootSection(F("Memory Status"));
    DebugManager::printf(DebugCategory::Boot, "Free RAM:         %u bytes\n", ESP.getFreeHeap());
    DebugManager::printf(DebugCategory::Boot, "Max Alloc Block:  %u bytes\n", ESP.getMaxAllocHeap());
    DebugManager::printf(DebugCategory::Boot, "Total Heap:       %u bytes\n", ESP.getHeapSize());
    DebugManager::printf(DebugCategory::Boot, "Min Free ever:    %u bytes\n", ESP.getMinFreeHeap());
    DebugManager::println(DebugCategory::Boot, "Effects with lazy-alloc: BouncingBalls, Fire2D, MatrixRain");

    logBootSection(F("Startup Animation (non-blocking)"));
}

static void configureMqttCallbacks() {
    mqttManager.setCallback([](const String& topic, const String& payload) {
        DynamicJsonDocument doc(1024);
        if (deserializeJson(doc, payload)) {
            DebugManager::println(DebugCategory::MQTT, "[MQTT][WARN] Failed to parse control payload");
            return;
        }

        ControlUpdate update = parseMqttControlUpdate(doc);
        enqueuePendingMqttControl(update);
    });
    g_mqttManager = &mqttManager;
}

static void enqueuePendingMqttControl(const ControlUpdate& update) {
    const bool replacingPending = gPendingMqttControl.pending;

    if (!replacingPending) {
        gPendingMqttControl.update = update;
    } else {
        if (update.hasPower) {
            gPendingMqttControl.update.hasPower = true;
            gPendingMqttControl.update.newPower = update.newPower;
        }
        if (update.hasBrightness) {
            gPendingMqttControl.update.hasBrightness = true;
            gPendingMqttControl.update.newBrightness = update.newBrightness;
        }
        if (update.hasColor) {
            gPendingMqttControl.update.hasColor = true;
            gPendingMqttControl.update.newColor = update.newColor;
        }
        if (update.hasEffect) {
            gPendingMqttControl.update.hasEffect = true;
            gPendingMqttControl.update.newEffect = update.newEffect;
        }
    }

    gPendingMqttControl.queuedAtMs = millis();
    gPendingMqttControl.sequence = gNextMqttControlSequence++;
    gPendingMqttControl.pending = true;

    if (replacingPending) {
        DebugManager::printf(DebugCategory::MQTT,
                             "[MQTT][QUEUE] Merged pending control into seq=%lu power=%u bright=%u color=%u effect=%u\n",
                             (unsigned long)gPendingMqttControl.sequence,
                             gPendingMqttControl.update.hasPower,
                             gPendingMqttControl.update.hasBrightness,
                             gPendingMqttControl.update.hasColor,
                             gPendingMqttControl.update.hasEffect);
    }
}

static void processPendingMqttControl() {
    if (!gPendingMqttControl.pending) {
        return;
    }

    const unsigned long nowMs = millis();
    if (nowMs - gPendingMqttControl.queuedAtMs < MQTT_CONTROL_SETTLE_MS) {
        return;
    }

    const PendingMqttControl queued = gPendingMqttControl;
    gPendingMqttControl.pending = false;

    const unsigned long queueDelayMs = nowMs - queued.queuedAtMs;
    const uint32_t applyStartUs = micros();

    applyControlUpdate(
        queued.update.hasPower, queued.update.newPower,
        queued.update.hasBrightness, queued.update.newBrightness,
        queued.update.hasColor, queued.update.newColor,
        queued.update.hasEffect, queued.update.newEffect,
        true,
        true,
        true
    );

    const uint32_t applyDurationUs = micros() - applyStartUs;
    if (queueDelayMs >= MQTT_QUEUE_WARN_MS || applyDurationUs >= MQTT_APPLY_WARN_US) {
        DebugManager::printf(
            DebugCategory::MQTT,
            "[MQTT][LATENCY] seq=%lu queue=%lums apply=%luus power=%u bright=%u color=%u effect=%u\n",
            (unsigned long)queued.sequence,
            queueDelayMs,
            (unsigned long)applyDurationUs,
            queued.update.hasPower,
            queued.update.hasBrightness,
            queued.update.hasColor,
            queued.update.hasEffect
        );
    }
}

static void updateBootSequence() {
    const unsigned long now = millis();

    if (gBootPhase == BootPhase::Done) {
        gBootActive = false;
        clearMatrix();
        strip->show();
        if (powerState) {
            handleCurrentEffect();
        }
        logBootSection(F("Boot Completed"));
        return;
    }

    switch (gBootPhase) {
        case BootPhase::StartupSweep: {
            if (gBootLastStepMs != 0 && (now - gBootLastStepMs) < 55) {
                return;
            }
            gBootLastStepMs = now;

            int x = gBootStartupColumn;
            uint16_t hue = (uint16_t)((uint32_t)x * 65535 / WIDTH);
            uint32_t c = ledMatrix.colorHSV(hue, 255, 230);
            uint8_t r = (c >> 16) & 0xFF;
            uint8_t g = (c >> 8) & 0xFF;
            uint8_t b = c & 0xFF;
            for (int y = 0; y < HEIGHT; y++) {
                strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
            }
            strip->show();

            gBootStartupColumn++;
            if (gBootStartupColumn >= WIDTH) {
                gBootPhase = BootPhase::StartupHold;
                gBootPhaseStartMs = now;
                gBootLastStepMs = 0;
            }
            return;
        }

        case BootPhase::StartupHold: {
            if (now - gBootPhaseStartMs >= 400) {
                gBootPhase = BootPhase::StartupFade;
                gBootStartupFadeStep = 250;
                gBootLastStepMs = 0;
            }
            return;
        }

        case BootPhase::StartupFade: {
            if (gBootLastStepMs != 0 && (now - gBootLastStepMs) < 18) {
                return;
            }
            gBootLastStepMs = now;

            float f = gBootStartupFadeStep / 255.0f;
            for (int x = 0; x < WIDTH; x++) {
                uint16_t hue = (uint16_t)((uint32_t)x * 65535 / WIDTH);
                uint32_t c = ledMatrix.colorHSV(hue, 255, 230);
                uint8_t r = (uint8_t)(((c >> 16) & 0xFF) * f);
                uint8_t g = (uint8_t)(((c >> 8) & 0xFF) * f);
                uint8_t b = (uint8_t)((c & 0xFF) * f);
                for (int y = 0; y < HEIGHT; y++) {
                    strip->setPixelColor(XY(x, y), makeColorWithBrightness(r, g, b));
                }
            }
            strip->show();

            gBootStartupFadeStep -= 10;
            if (gBootStartupFadeStep < 0) {
                clearMatrix();
                strip->show();
                gBootPhase = BootPhase::WifiAnim;
                gBootLastStepMs = 0;
                gBootWifiFrames = 0;
                gBootWifiTargetFrames = bootWifiAnimOnePassFrames();
                fxSetupWifi.reset();
                logBootSection(F("WiFi Connect Animation"));
            }
            return;
        }

        case BootPhase::WifiAnim: {
            if (gBootLastStepMs != 0 && (now - gBootLastStepMs) < 65) {
                return;
            }
            gBootLastStepMs = now;
            fxSetupWifi.update();
            gBootWifiFrames++;
            if (gBootWifiFrames >= gBootWifiTargetFrames) {
                gBootPhase = BootPhase::WifiConnect;
                logBootSection(F("WiFi Connect"));
            }
            return;
        }

        case BootPhase::WifiConnect: {
            wifiManager.connectToWiFi();
            if (wifiManager.isSetupMode()) {
                gBootActive = false;
                return;
            }
            gBootPhase = BootPhase::NetInit;
            return;
        }

        case BootPhase::NetInit: {
            logBootSection(F("NTP / OTA"));
            timeManager.setTimezone("CET-1CEST,M3.5.0,M10.5.0/3");
            ArduinoOTA.setHostname("WordClock");
            ArduinoOTA.setPassword("update123");
            ArduinoOTA.begin();

            gBootMqttConfigured = !wifiManager.getMQTTServer().isEmpty();
            if (gBootMqttConfigured) {
                mqttManager.setConfig(wifiManager.getMQTTServer(), wifiManager.getMQTTPort(),
                                      wifiManager.getMQTTUser(), wifiManager.getMQTTPassword());
                configureMqttCallbacks();
                gBootMqttRingFx.reset();
                gBootMqttFrames = 0;
                gBootLastStepMs = 0;
                gBootPhase = gBootSuppressVisuals ? BootPhase::MqttConnect : BootPhase::MqttAnim;
                if (!gBootSuppressVisuals) {
                    logBootSection(F("MQTT Load Animation"));
                }
            } else {
                gBootMinuteDotIndex = 0;
                gBootLastStepMs = 0;
                gBootPhase = gBootSuppressVisuals ? BootPhase::Done : BootPhase::MqttFallbackDots;
                if (!gBootSuppressVisuals) {
                    logBootSection(F("MQTT Missing -> Minute LED Loader"));
                }
            }
            return;
        }

        case BootPhase::MqttAnim: {
            if (gBootLastStepMs != 0 && (now - gBootLastStepMs) < 65) {
                return;
            }
            gBootLastStepMs = now;
            gBootMqttRingFx.update();
            gBootMqttFrames++;
            if (gBootMqttFrames >= 34) {
                gBootPhase = BootPhase::MqttConnect;
                logBootSection(F("MQTT Connect"));
            }
            return;
        }

        case BootPhase::MqttConnect: {
            mqttManager.connect();
            gBootPhase = BootPhase::Done;
            return;
        }

        case BootPhase::MqttFallbackDots: {
            if (gBootLastStepMs != 0 && (now - gBootLastStepMs) < 500) {
                return;
            }
            gBootLastStepMs = now;

            clearMatrix();
            if (gBootMinuteDotIndex < 4) {
                const uint32_t dotOn = makeColorWithBrightness(0, 170, 255);
                strip->setPixelColor(XY(7 + gBootMinuteDotIndex, 9), dotOn);
                strip->show();
                gBootMinuteDotIndex++;
            } else {
                clearMatrix();
                strip->show();
                gBootPhase = BootPhase::Done;
            }
            return;
        }

        case BootPhase::Done:
        default:
            return;
    }
}


// ---------------------------------------------------------
// LOOP
// ---------------------------------------------------------
void loop() {
    const unsigned long loopStartMs = millis();
    static unsigned long sBootAtMs = millis();
    static unsigned long sLastOtaAutoCheckMs = 0;
    static bool sOtaFirstCheckDone = false;
    // =====================================================================
    // FRAME TIMING: Limit LED rendering to ~60 Hz for smooth visuals
    // Network I/O runs EVERY loop iteration regardless!
    // =====================================================================
    static unsigned long _lastFrameTime = 0;
    const uint32_t FRAME_TIME_MS = 16;  // ~60 Hz
    unsigned long _nowMs = millis();
    bool renderFrame = (_nowMs - _lastFrameTime >= FRAME_TIME_MS);
    if (renderFrame) _lastFrameTime = _nowMs;

    // =====================================================================
    // PRIORITY 1: Handle serial (quick, local)
    // =====================================================================
    handleSerialCommands();

    if (gBootActive) {
        if (renderFrame) updateBootSequence();
        // Network I/O below still runs during boot
    } else if (wifiManager.isSetupMode()) {
        // =====================================================================
        // PRIORITY 2: SETUP MODE
        // =====================================================================
        wifiManager.handleSetup();
        if (renderFrame) fxSetupWifi.update();
    } else if (renderFrame) {
        // =====================================================================
        // PRIORITY 3: EFFECT RENDERING (only on frame tick)
        // =====================================================================
        if (!powerState) {
            clearMatrix();
            strip->show();
        } else {
            handleCurrentEffect();
        }
    }

    // =====================================================================
    // PRIORITY 4: Network I/O – runs EVERY loop iteration
    // =====================================================================
    mqttManager.loop();
    processPendingMqttControl();
    wifiManager.getServer()->handleClient();
    ArduinoOTA.handle();
    processPendingMqttColor();
    stateManager.processPendingSave();

    // =====================================================================
    // PRIORITY 5: Time & Memory Management (periodic, low-frequency)
    // =====================================================================
    static unsigned long _lastTimeSync = 0;
    if (millis() - _lastTimeSync >= 1000) {  // Every 1 second
        _lastTimeSync = millis();
        timeManager.updateNTPSync();
        syncRtcFromNtpIfNeeded();
        rtcManager.updateHealth();
    }

    // Memory check (periodic crisis handler)
    static unsigned long _lastMemoryCheck = 0;
    if (millis() - _lastMemoryCheck >= 5000) {  // Check every 5 seconds
        _lastMemoryCheck = millis();
        if (MemoryManager::shouldFallbackToSafeEffect(powerState, currentEffect)) {
            MemoryManager::logFallbackAction(currentEffect, "clock");
            transitionToEffect("clock");
        }
    }

    // OTA auto-check runs only outside setup mode and after boot stabilization.
    const unsigned long nowMs = millis();
    const bool otaContextOk = WiFi.isConnected() && !wifiManager.isSetupMode() && !gBootActive;
    if (otaContextOk) {
        if (!sOtaFirstCheckDone && (nowMs - sBootAtMs >= OTA_FIRST_CHECK_DELAY_MS)) {
            sOtaFirstCheckDone = true;
            sLastOtaAutoCheckMs = nowMs;
            checkForUpdateAndInstall(false);
        } else if (sOtaFirstCheckDone && (nowMs - sLastOtaAutoCheckMs >= OTA_AUTO_CHECK_INTERVAL_MS)) {
            sLastOtaAutoCheckMs = nowMs;
            checkForUpdateAndInstall(false);
        }
    }

    const unsigned long loopDurationMs = millis() - loopStartMs;
    if (loopDurationMs >= MAIN_LOOP_WARN_MS && millis() - gLastLoopDurationWarnAt >= 1000) {
        gLastLoopDurationWarnAt = millis();
        DebugManager::printf(DebugCategory::Loop,
                             "[LOOP][WARN] Main loop took %lums (mqtt=%u wifi=%u boot=%u effect=%s)\n",
                             loopDurationMs,
                             mqttManager.isConnected() ? 1 : 0,
                             WiFi.isConnected() ? 1 : 0,
                             gBootActive ? 1 : 0,
                             currentEffect.c_str());
    }

    const unsigned long now = millis();
    if (DebugManager::isEnabled(DebugCategory::Loop) && (now - gLastLoopHeartbeatMs >= LOOP_HEARTBEAT_MS)) {
        gLastLoopHeartbeatMs = now;
        DebugManager::printf(DebugCategory::Loop,
                             "[LOOP] alive effect=%s last_fx=%luus max_fx=%luus samples=%lu mqtt=%u wifi=%u\n",
                             currentEffect.c_str(),
                             (unsigned long)gLastEffectUpdateUs,
                             (unsigned long)gMaxEffectUpdateUsWindow,
                             (unsigned long)gEffectUpdateSamples,
                             mqttManager.isConnected() ? 1 : 0,
                             WiFi.isConnected() ? 1 : 0);
        gMaxEffectUpdateUsWindow = 0;
        gEffectUpdateSamples = 0;
    }
}


// ---------------------------------------------------------
// Effekt-Router (Clock ist jetzt ein echter Effekt)
// ---------------------------------------------------------
void handleCurrentEffect() {

    // CLOCK EFFECT
    if (currentEffect == "clock") {
        static int lastMinute = -1;
        static unsigned long lastSecondlyRender = 0;
        static unsigned long lastMqttPublish = 0;

        struct tm timeinfo;
        int displayHour = -1;
        int displayMinute = -1;

        if (getLocalTime(&timeinfo)) {
            displayHour = timeinfo.tm_hour;
            displayMinute = timeinfo.tm_min;
        } else if (rtcManager.isAvailable()) {
            DateTime rtcNow = rtcManager.getTime();
            displayHour = rtcNow.hour();
            displayMinute = rtcNow.minute();
        }

        // Render when minute changes (new time) OR every second to maintain display
        unsigned long nowMs = millis();
        bool minuteChanged = (displayMinute >= 0 && displayMinute != lastMinute);
        bool shouldRenderSecondly = (nowMs - lastSecondlyRender >= 1000);

        if (minuteChanged) {
            const uint32_t effectStartUs = micros();
            showTime(displayHour, displayMinute);
            const uint32_t effectDurationUs = micros() - effectStartUs;
            gLastEffectUpdateUs = effectDurationUs;
            gEffectUpdateSamples++;
            if (effectDurationUs > gMaxEffectUpdateUsWindow) {
                gMaxEffectUpdateUsWindow = effectDurationUs;
            }
            if (effectDurationUs >= EFFECT_UPDATE_WARN_US) {
                DebugManager::printf(DebugCategory::Effects,
                                     "[EFFECT][WARN] clock(showTime) took %luus\n",
                                     (unsigned long)effectDurationUs);
            }
            lastMinute = displayMinute;
            lastSecondlyRender = nowMs;  // Reset secondly timer when minute changes
            DebugManager::println(DebugCategory::Main, String(displayHour) + ":" + String(displayMinute));
        } else if (shouldRenderSecondly && displayMinute >= 0) {
            // Re-render clock every second to prevent dropout (and render warning)
            const uint32_t effectStartUs = micros();
            showTime(displayHour, displayMinute);
            const uint32_t effectDurationUs = micros() - effectStartUs;
            gLastEffectUpdateUs = effectDurationUs;
            gEffectUpdateSamples++;
            if (effectDurationUs > gMaxEffectUpdateUsWindow) {
                gMaxEffectUpdateUsWindow = effectDurationUs;
            }
            if (effectDurationUs >= EFFECT_UPDATE_WARN_US) {
                DebugManager::printf(DebugCategory::Effects,
                                     "[EFFECT][WARN] clock(showTime) took %luus\n",
                                     (unsigned long)effectDurationUs);
            }
            lastSecondlyRender = nowMs;
        }

        // Publish MQTT state when minute changes (not every second)
        if (minuteChanged && nowMs - lastMqttPublish >= 5000) {
            lastMqttPublish = nowMs;
            mqttManager.publishState(powerState, currentEffect, color, brightness);
        }

        // RTC health warning: blink in corner if available
        if (displayMinute >= 0 && rtcManager.hasHealthWarning()) {
            static unsigned long lastBlinkMs = 0;
            static bool blinkOn = false;
            if (nowMs - lastBlinkMs >= 500) {
                lastBlinkMs = nowMs;
                blinkOn = !blinkOn;
            }

            uint32_t warnColor = blinkOn ? makeColorWithBrightness(255, 30, 0) : 0;
            strip->setPixelColor(XY(WIDTH - 1, 0), warnColor);
            strip->setPixelColor(XY(WIDTH - 2, 0), warnColor);
            strip->show();  // Show warning
        } else if (displayMinute >= 0) {
            // Normal clock display already showed via showTime()
            // Don't call strip->show() again - it's already been called
        }
        return;
    }

    // Andere Effekte – über Effect-Klassen dispatchen
    for (Effect* e : allEffects) {
        if (currentEffect == e->name()) {
            const uint32_t effectStartUs = micros();
            e->update();
            const uint32_t effectDurationUs = micros() - effectStartUs;
            gLastEffectUpdateUs = effectDurationUs;
            gEffectUpdateSamples++;
            if (effectDurationUs > gMaxEffectUpdateUsWindow) {
                gMaxEffectUpdateUsWindow = effectDurationUs;
            }
            if (effectDurationUs >= EFFECT_UPDATE_WARN_US) {
                DebugManager::printf(DebugCategory::Effects,
                                     "[EFFECT][WARN] %s update took %luus\n",
                                     currentEffect.c_str(),
                                     (unsigned long)effectDurationUs);
            }
            // e->update() calls strip->show() internally
            return;
        }
    }

    // Unknown runtime effect: fallback to clock to avoid black screen.
    DebugManager::print(DebugCategory::Main, "Unknown runtime effect, fallback to clock: ");
    DebugManager::println(DebugCategory::Main, currentEffect);
    transitionToEffect("clock");
}

static void syncRtcFromNtpIfNeeded() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return;
    }

    static int lastRtcSyncHour = -1;
    bool shouldSyncRtc = false;

    // Sync once after boot as soon as NTP is valid.
    if (lastRtcSyncHour < 0) {
        shouldSyncRtc = true;
    }
    // Then sync at each full hour, once per hour.
    else if (timeinfo.tm_min == 0 && timeinfo.tm_hour != lastRtcSyncHour) {
        shouldSyncRtc = true;
    }

    if (!shouldSyncRtc) {
        return;
    }

    DateTime ntpNow(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );
    rtcManager.setTime(ntpNow);
    lastRtcSyncHour = timeinfo.tm_hour;
}

static void processPendingMqttColor() {
    if (!gHasPendingMqttColor) {
        return;
    }
    const unsigned long now = millis();
    if (now - gPendingMqttColorAt < MQTT_COLOR_SETTLE_MS) {
        return;
    }
    if (now - gLastAppliedMqttColorAt < MQTT_COLOR_MIN_APPLY_INTERVAL_MS) {
        return;
    }

    gHasPendingMqttColor = false;

    if (gPendingMqttColor == color) {
        return;
    }

    stateManager.setColor(gPendingMqttColor);
    color = gPendingMqttColor;
    gLastAppliedMqttColorAt = now;

    if (!powerState) {
        return;
    }

    if (currentEffect == "clock") {
        // Color changes should not trigger a long time-morph animation.
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            resetClockMorphState();
            showTime(timeinfo.tm_hour, timeinfo.tm_min);
        }
    }

    if (g_mqttManager) {
        g_mqttManager->publishState(powerState, currentEffect, color, brightness);
    }
}


// ---------------------------------------------------------
// Testfunktion
// ---------------------------------------------------------
void runFullTimeTest() {
    DebugManager::println(DebugCategory::Test, "Starte kompletten WordClock-Testlauf...");
    showTime(1, 0);
    waitMs(3000);
    for (int hour = 0; hour < 12; hour++) {
        showTime(hour, 0);
        strip->show();
        waitMs(250);
    }

    for (int minute = 0; minute < 60; minute++) {
        showTime(5, minute);
        strip->show();
        waitMs(250);
    }

    DebugManager::println(DebugCategory::Test, "WordClock-Testlauf abgeschlossen.");
}

void runSmokeTest() {
    DebugManager::println(DebugCategory::Test, "Starte Smoke-Test...");

    applyControlUpdate(
        true, true,
        true, 160,
        true, 0x00CC66,
        true, "clock",
        false,
        true,
        true
    );
    waitMs(500);

    applyControlUpdate(
        false, powerState,
        false, brightness,
        false, color,
        true, "matrix",
        false,
        true,
        true
    );
    waitMs(800);

    transitionMs = 1200;
    effectSpeed = 65;
    effectIntensity = 70;

    applyControlUpdate(
        false, powerState,
        false, brightness,
        true, 0xFF5500,
        false, "",
        false,
        true,
        true
    );
    waitMs(500);

    applyControlUpdate(
        false, powerState,
        false, brightness,
        false, color,
        true, "clock",
        false,
        true,
        true
    );

    DebugManager::println(DebugCategory::Test, "Smoke-Test abgeschlossen.");
}

void runSelfTest() {
    DebugManager::println(DebugCategory::Test, "[SELFTEST] Starting regression test...");

    bool allPass = true;
    uint8_t failures = 0;
    auto assertCheck = [&](bool condition, const char* label) {
        DebugManager::print(DebugCategory::Test, "[SELFTEST] ");
        DebugManager::print(DebugCategory::Test, label);
        DebugManager::print(DebugCategory::Test, ": ");
        DebugManager::println(DebugCategory::Test, condition ? "PASS" : "FAIL");
        if (!condition) {
            allPass = false;
            failures++;
        }
    };

    const bool oldPower = powerState;
    const uint8_t oldBrightness = brightness;
    const uint32_t oldColor = color;
    const String oldEffect = currentEffect;
    const uint8_t oldSpeed = effectSpeed;
    const uint8_t oldIntensity = effectIntensity;
    const uint16_t oldTransition = transitionMs;

    applyControlUpdate(
        false, powerState,
        false, brightness,
        false, color,
        true, "matrix",
        false,
        true,
        true
    );
    assertCheck(currentEffect == "matrix", "effect switch clock->matrix");

    applyControlUpdate(
        false, powerState,
        false, brightness,
        false, color,
        true, "clock",
        false,
        true,
        true
    );
    assertCheck(currentEffect == "clock", "effect switch matrix->clock");

    effectSpeed = 77;
    effectIntensity = 33;
    transitionMs = 2100;
    stateManager.setSpeed(effectSpeed);
    stateManager.setIntensity(effectIntensity);
    stateManager.setTransitionMs(transitionMs);
    stateManager.saveToPreferences();
    assertCheck(effectSpeed == 77 && effectIntensity == 33 && transitionMs == 2100, "set tuning values");

    effectSpeed = ControlConfig::DEFAULT_SPEED;
    effectIntensity = ControlConfig::DEFAULT_INTENSITY;
    transitionMs = ControlConfig::DEFAULT_TRANSITION_MS;
    stateManager.setSpeed(effectSpeed);
    stateManager.setIntensity(effectIntensity);
    stateManager.setTransitionMs(transitionMs);
    stateManager.saveToPreferences();
    assertCheck(
        effectSpeed == ControlConfig::DEFAULT_SPEED &&
        effectIntensity == ControlConfig::DEFAULT_INTENSITY &&
        transitionMs == ControlConfig::DEFAULT_TRANSITION_MS,
        "reset defaults"
    );

    if (g_mqttManager && g_mqttManager->isConnected()) {
        g_mqttManager->publish("wordclock/selftest", allPass ? "PASS" : "FAIL");
        DebugManager::println(DebugCategory::Test, "[SELFTEST] mqtt publish marker: PASS");
    } else {
        DebugManager::println(DebugCategory::Test, "[SELFTEST] mqtt publish marker: SKIP (not connected)");
    }

    applyControlUpdate(
        true, oldPower,
        true, oldBrightness,
        true, oldColor,
        true, oldEffect,
        false,
        true,
        true
    );
    effectSpeed = oldSpeed;
    effectIntensity = oldIntensity;
    transitionMs = oldTransition;
    stateManager.setSpeed(effectSpeed);
    stateManager.setIntensity(effectIntensity);
    stateManager.setTransitionMs(transitionMs);
    stateManager.saveToPreferences();

    DebugManager::printf(DebugCategory::Test, "[SELFTEST] Completed: %s (failures=%u)\n", allPass ? "PASS" : "FAIL", failures);
}
