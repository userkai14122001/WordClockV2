#include "WordClockLayoutPresets.h"

#include "effects.h"

namespace {
static const char* kTextAumovioHero =
    "ESSISTTFÜNF\n"
    "ZEHNZWANZIG\n"
    "DREIVIERTEL\n"
    "VORAUMONACH\n"
    "HALBEELFÜNF\n"
    "DREIVIOVIER\n"
    "SECHSSIEBEN\n"
    "ZEHNEUNZWEI\n"
    "AACHTZWÖLFF\n"
    "EINSUHR****";

static const char* kTextDefaultKai =
    "ESSISTTFÜNF\n"
    "ZEHNZWANZIG\n"
    "DREIVIERTEL\n"
    "VORLOVENACH\n"
    "HALBEELFÜNF\n"
    "DREIYOUVIER\n"
    "SECHSSIEBEN\n"
    "ZEHNEUNZWEI\n"
    "AACHTZWÖLFF\n"
    "EINSUHR****";

static void applySharedWordPositions() {
    setClockWordPosition("ES", {0, 0, 2});
    setClockWordPosition("IST", {3, 0, 3});
    setClockWordPosition("FUENF", {7, 0, 4});
    setClockWordPosition("ZEHN", {0, 1, 4});
    setClockWordPosition("ZWANZIG", {4, 1, 7});
    setClockWordPosition("VIERTEL", {4, 2, 7});
    setClockWordPosition("VOR", {0, 3, 3});
    setClockWordPosition("NACH", {7, 3, 4});
    setClockWordPosition("HALB", {0, 4, 4});

    setClockWordPosition("ZWOELF", {5, 8, 5});
    setClockWordPosition("EINS", {0, 9, 4});
    setClockWordPosition("EIN", {0, 9, 3});
    setClockWordPosition("ZWEI", {7, 7, 4});
    setClockWordPosition("DREI", {0, 5, 4});
    setClockWordPosition("VIER", {7, 5, 4});
    setClockWordPosition("FUENF_H", {7, 4, 4});
    setClockWordPosition("SECHS", {0, 6, 5});
    setClockWordPosition("SIEBEN", {5, 6, 6});
    setClockWordPosition("ACHT", {1, 8, 4});
    setClockWordPosition("NEUN", {3, 7, 4});
    setClockWordPosition("ZEHN_H", {0, 7, 4});
    setClockWordPosition("ELF", {5, 4, 3});

    setClockWordPosition("UHR", {4, 9, 3});
    setClockWordPosition("M1", {7, 9, 1});
    setClockWordPosition("M2", {8, 9, 1});
    setClockWordPosition("M3", {9, 9, 1});
    setClockWordPosition("M4", {10, 9, 1});
    setClockWordPosition("LOVE", {3, 3, 4});
    setClockWordPosition("YOU", {4, 5, 3});
}

static void applyAumovioHeroWordPositions() {
    applySharedWordPositions();
}

static void applyDefaultKaiWordPositions() {
    applySharedWordPositions();
}

static const WordClockLayoutPreset kPresets[] = {
    {"hero", "Aumovio", kTextAumovioHero, applyAumovioHeroWordPositions},
    {"kai", "Kai", kTextDefaultKai, applyDefaultKaiWordPositions},
};

static constexpr size_t kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);
}

const WordClockLayoutPreset* wordClockLayoutDefaultPreset() {
    return &kPresets[1];
}

const WordClockLayoutPreset* wordClockLayoutFindPreset(const String& id) {
    for (size_t i = 0; i < kPresetCount; i++) {
        if (id == kPresets[i].id) {
            return &kPresets[i];
        }
    }
    return nullptr;
}

bool wordClockLayoutIsPresetId(const String& id) {
    return wordClockLayoutFindPreset(id) != nullptr;
}
