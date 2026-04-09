#pragma once

#include <Arduino.h>

struct WordClockLayoutPreset {
    const char* id;
    const char* name;
    const char* text;
    void (*applyWordPositions)();
};

const WordClockLayoutPreset* wordClockLayoutDefaultPreset();
const WordClockLayoutPreset* wordClockLayoutFindPreset(const String& id);
bool wordClockLayoutIsPresetId(const String& id);
