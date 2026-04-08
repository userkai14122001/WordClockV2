#include "WordClockLayout.h"

#include <Preferences.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "effects.h"
#include "DebugManager.h"

namespace {
static const char* kPrefsNs     = "layout";
static const char* kPrefsActive = "active";
static const char* kPrefsName   = "name";
static const char* kCustomPath  = "/layout_custom.json";
static const char* kDefaultName = "Standard";
static const char* kDefaultText =
    "ESXISTXFUEN\n"
    "ZEHNZWANZIG\n"
    "XXXXVIERTEL\n"
    "VORLOVENACH\n"
    "HALBXELFUEN\n"
    "DREIYOUVIER\n"
    "SECHSSIEBEN\n"
    "ZEHNEUNZWEI\n"
    "XACHTZWOLFX\n"
    "EINSUHR****";

static const char* kKeys[] = {
    "ES", "IST", "FUENF", "ZEHN", "ZWANZIG", "VIERTEL", "VOR", "NACH", "HALB",
    "ZWOELF", "EINS", "EIN", "ZWEI", "DREI", "VIER", "FUENF_H", "SECHS", "SIEBEN",
    "ACHT", "NEUN", "ZEHN_H", "ELF", "UHR", "M1", "M2", "M3", "M4", "LOVE", "YOU"
};
static const size_t kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

static String gActiveLayoutId   = "default";
static String gActiveLayoutName = kDefaultName;
static String gLayoutText       = kDefaultText;

static bool parseWord(JsonVariant value, Word& out) {
    if (!value.is<JsonArray>()) return false;
    JsonArray a = value.as<JsonArray>();
    if (a.size() != 3) return false;
    int x = a[0] | -1;
    int y = a[1] | -1;
    int len = a[2] | -1;
    if (x < 0 || y < 0 || len <= 0) return false;
    out = {x, y, len};
    return true;
}

static bool normalizeLayoutText(const String& in, String& out) {
    String s = in;
    s.replace("\r\n", "\n");
    s.replace('\r', '\n');
    while (s.endsWith("\n")) {
        s.remove(s.length() - 1);
    }

    int lineCount = 1;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == '\n') lineCount++;
    }
    if (lineCount != 10) {
        return false;
    }

    int start = 0;
    for (int line = 0; line < 10; line++) {
        int end = s.indexOf('\n', start);
        if (end < 0) end = s.length();
        String one = s.substring(start, end);
        if (one.length() != 11) {
            return false;
        }
        start = end + 1;
    }

    out = s;
    return true;
}

static bool applyWordsFromJson(JsonObject words, String& error) {
    for (size_t i = 0; i < kKeyCount; i++) {
        const char* key = kKeys[i];
        JsonVariant node = words[key];
        Word w;
        if (!parseWord(node, w)) {
            error = String("Ungueltige Wortposition fuer ") + key;
            return false;
        }
        if (!setClockWordPosition(String(key), w)) {
            error = String("Wortschluessel unbekannt: ") + key;
            return false;
        }
    }
    return true;
}

static void saveActiveId(const String& id, const String& name) {
    Preferences prefs;
    if (!prefs.begin(kPrefsNs, false)) return;
    prefs.putString(kPrefsActive, id);
    prefs.putString(kPrefsName, name);
    prefs.end();
}

static String loadActiveId() {
    Preferences prefs;
    if (!prefs.begin(kPrefsNs, true)) return "default";
    String id = prefs.getString(kPrefsActive, "default");
    prefs.end();
    id.trim();
    id.toLowerCase();
    return (id == "custom") ? "custom" : "default";
}

static String loadActiveName(const String& id) {
    Preferences prefs;
    if (!prefs.begin(kPrefsNs, true)) return (id == "custom") ? "Custom" : kDefaultName;
    String name = prefs.getString(kPrefsName, (id == "custom") ? "Custom" : kDefaultName);
    prefs.end();
    name.trim();
    if (name.isEmpty()) name = (id == "custom") ? "Custom" : kDefaultName;
    return name;
}

static bool loadCustomFromFile(String& textOut, String& nameOut, String& err) {
    File f = SPIFFS.open(kCustomPath, "r");
    if (!f) {
        err = "Custom-Layout-Datei fehlt";
        return false;
    }
    String payload = f.readString();
    f.close();

    DynamicJsonDocument doc(4096);
    auto e = deserializeJson(doc, payload);
    if (e) {
        err = String("Custom-Layout JSON Fehler: ") + e.c_str();
        return false;
    }

    String text = doc["text"] | "";
    String normalized;
    if (!normalizeLayoutText(text, normalized)) {
        err = "Layout-Text muss exakt 10 Zeilen mit je 11 Zeichen haben";
        return false;
    }

    JsonObject words = doc["words"].as<JsonObject>();
    if (words.isNull()) {
        err = "words Objekt fehlt";
        return false;
    }

    if (!applyWordsFromJson(words, err)) {
        return false;
    }

    textOut = normalized;
    nameOut = doc["name"] | "Custom";
    if (nameOut.isEmpty()) nameOut = "Custom";
    return true;
}

static String buildPositionsJson() {
    DynamicJsonDocument doc(1536);
    JsonObject obj = doc.to<JsonObject>();
    for (size_t i = 0; i < kKeyCount; i++) {
        Word w;
        if (!getClockWordPosition(String(kKeys[i]), w)) continue;
        JsonArray a = obj.createNestedArray(kKeys[i]);
        a.add(w.x);
        a.add(w.y);
        a.add(w.len);
    }
    String out;
    serializeJsonPretty(doc, out);
    return out;
}
}

void wordClockLayoutInit() {
    gActiveLayoutId = loadActiveId();
    resetClockWordPositionsToDefault();

    if (gActiveLayoutId == "custom") {
        String text;
        String name;
        String err;
        if (loadCustomFromFile(text, name, err)) {
            gLayoutText = text;
            gActiveLayoutName = name;
        } else {
            DebugManager::print(DebugCategory::Effects, "[LAYOUT] Custom laden fehlgeschlagen: ");
            DebugManager::println(DebugCategory::Effects, err);
            gActiveLayoutId   = "default";
            gActiveLayoutName = kDefaultName;
            gLayoutText       = kDefaultText;
            saveActiveId(gActiveLayoutId, gActiveLayoutName);
        }
    } else {
        gActiveLayoutName = kDefaultName;
        gLayoutText       = kDefaultText;
    }
}

const String& wordClockLayoutActiveId() {
    return gActiveLayoutId;
}

const String& wordClockLayoutActiveName() {
    return gActiveLayoutName;
}

const String& wordClockLayoutText() {
    return gLayoutText;
}

String wordClockLayoutWordPositionsJson() {
    return buildPositionsJson();
}

bool wordClockLayoutApplyAndStore(const String& layoutId,
                                  const String& layoutName,
                                  const String& layoutText,
                                  const String& positionsJson,
                                  String& error) {
    String id = layoutId;
    id.trim();
    id.toLowerCase();
    if (id != "default" && id != "custom") {
        error = "layout_id muss default oder custom sein";
        return false;
    }

    String name = layoutName;
    name.trim();
    if (name.isEmpty()) name = (id == "custom") ? "Custom" : kDefaultName;

    if (id == "default") {
        resetClockWordPositionsToDefault();
        resetClockMorphState();
        gActiveLayoutId   = "default";
        gActiveLayoutName = kDefaultName;
        gLayoutText       = kDefaultText;
        saveActiveId(gActiveLayoutId, gActiveLayoutName);
        return true;
    }

    String normalizedText;
    if (!normalizeLayoutText(layoutText, normalizedText)) {
        error = "Layout-Text muss exakt 10 Zeilen mit je 11 Zeichen haben";
        return false;
    }

    DynamicJsonDocument doc(4096);
    auto e = deserializeJson(doc, positionsJson);
    if (e) {
        error = String("Positions-JSON ungueltig: ") + e.c_str();
        return false;
    }
    JsonObject words = doc.as<JsonObject>();
    if (words.isNull()) {
        error = "Positions-JSON muss ein Objekt sein";
        return false;
    }

    resetClockWordPositionsToDefault();
    if (!applyWordsFromJson(words, error)) {
        resetClockWordPositionsToDefault();
        return false;
    }

    DynamicJsonDocument storeDoc(4096);
    storeDoc["name"]  = name;
    storeDoc["text"]  = normalizedText;
    storeDoc["words"] = words;
    String payload;
    serializeJson(storeDoc, payload);

    File f = SPIFFS.open(kCustomPath, "w");
    if (!f) {
        error = "Custom-Layout-Datei konnte nicht geschrieben werden";
        return false;
    }
    f.print(payload);
    f.close();

    gLayoutText = normalizedText;
    gActiveLayoutId   = "custom";
    gActiveLayoutName = name;
    saveActiveId(gActiveLayoutId, gActiveLayoutName);
    resetClockMorphState();
    return true;
}
