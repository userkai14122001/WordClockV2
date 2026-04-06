#pragma once

#include <Arduino.h>

void shutdownLedsForRestart();
void rebootDevice(const char* reason = nullptr, uint32_t delayMs = 120);
