#include "SystemControl.h"

#include <esp_system.h>

#include "DebugManager.h"
#include "matrix.h"

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

void shutdownLedsForRestart() {
    if (!strip) {
        return;
    }

    for (uint16_t i = 0; i < strip->numPixels(); i++) {
        strip->setPixelColor(i, 0);
    }
    strip->setBrightness(0);
    strip->show();
    waitMs(20);
}

void rebootDevice(const char* reason, uint32_t delayMs) {
    if (reason != nullptr && reason[0] != '\0') {
        DebugManager::printf(DebugCategory::Main, "[SYS] Software reboot: %s\n", reason);
    }

    shutdownLedsForRestart();

    if (delayMs > 0) {
        waitMs(delayMs);
    }

    esp_restart();
}
