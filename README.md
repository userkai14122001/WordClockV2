# WordClock

ESP32-C3-basierte Wortuhr mit 11×10 NeoPixel-Matrix, DS3231-RTC, WiFi-Setup-Portal, MQTT/Home-Assistant-Integration und mehreren Animationseffekten.

---

## Hardware

| Komponente | Details |
|---|---|
| Board | Seeed Studio XIAO ESP32-C3 |
| LED-Matrix | 11×10 WS2812B/NeoPixel (110 Pixel) |
| RTC | DS3231 |
| Framework | Arduino via PlatformIO |
| Flash | 4 MB, Partitionsschema mit SPIFFS |

---

## Projektstruktur

```text
WordClock/
├── platformio.ini
├── README.md
├── data/
│   ├── Pfote.png
│   └── ziffernblatt.svg
├── include/
│   ├── config.h
│   ├── ControlConfig.h
│   ├── DebugManager.h
│   ├── EffectManager.h
│   ├── LEDMatrix.h
│   ├── MemoryManager.h
│   ├── MQTTManager.h
│   ├── ota_https_update.h
│   ├── RTCManager.h
│   ├── rtc_module.hpp
│   ├── SerialCommands.h
│   ├── StateManager.h
│   ├── SystemControl.h
│   ├── TimeManager.h
│   ├── web_pages.h
│   └── WiFiManager.h
└── src/
    ├── main.cpp
    ├── config.cpp
    ├── DebugManager.cpp
    ├── EffectManager.cpp
    ├── effects.cpp
    ├── LEDMatrix.cpp
    ├── matrix.cpp
    ├── MemoryManager.cpp
    ├── MQTTManager.cpp
    ├── ota_https_update.cpp
    ├── RTCManager.cpp
    ├── rtc_module.cpp
    ├── SerialCommands.cpp
    ├── StateManager.cpp
    ├── SystemControl.cpp
    ├── TimeManager.cpp
    ├── web_pages.cpp
    ├── WiFiManager.cpp
    └── effects/
        ├── AuroraEffect.cpp
        ├── BouncingBallsEffect.cpp
        ├── ColorloopEffect.cpp
        ├── ColorwipeEffect.cpp
        ├── EnchantmentEffect.cpp
        ├── Fire2DEffect.cpp
        ├── InwardRippleEffect.cpp
        ├── LoveYouEffect.cpp
        ├── MatrixRainEffect.cpp
        ├── PlasmaEffect.cpp
        ├── SnakeEffect.cpp
        ├── StartupAnimation.cpp
        ├── TwinkleEffect.cpp
        ├── WaterDropEffect.cpp
        └── WifiRingEffect.cpp
```

---

## Build & Upload

`platformio.ini` enthält zwei Environments:

| Environment | Zweck |
|---|---|
| `seeed_xiao_esp32c3_usb` | USB-Upload (Standard) |
| `seeed_xiao_esp32c3_ota` | OTA-Upload über WLAN |

```bash
pio run                                            # kompilieren
pio run -t upload                                  # USB-Upload
pio run -e seeed_xiao_esp32c3_ota -t upload        # OTA-Upload
pio run -t uploadfs                                # SPIFFS-Filesystem flashen
pio device monitor                                 # Serial Monitor (115200 baud)
pio run -t clean                                   # Build-Artefakte löschen
```

---

## Laufzeitarchitektur

Die zentrale Laufzeit liegt in `src/main.cpp`. Alle Module sind als Klassen mit klarer Verantwortung aufgebaut:

| Modul | Verantwortung |
|---|---|
| `WiFiManager` | Setup-Portal, Web-API, WLAN-Verbindung |
| `MQTTManager` | Discovery, State-Publish, Tuning-Topics, Command-Handling |
| `RTCManager` | RTC-Zugriff und Gesundheitsstatus |
| `StateManager` | Persistenter Gerätezustand via NVS Preferences |
| `EffectManager` | Effekt-Registry, Wechsel und Übergänge |
| `TimeManager` | NTP-Synchronisierung und RTC-Abgleich |
| `DebugManager` | Laufzeit-Debug-Kategorien (ein-/ausschaltbar per Serial) |
| `LEDMatrix` | NeoPixel-Treiberebene |

---

## Steuerpfade

Alle Eingaben laufen über denselben Update-Pfad:

1. Quelle: MQTT (`wordclock/set`), Web-API oder Serial Command
2. Parsing in ein gemeinsames `ControlUpdate`-Struct
3. Bei MQTT-JSON: Coalescing innerhalb eines 60 ms Settle-Fensters (Merge-Queue)
4. Anwenden über `applyControlUpdate()`
5. Rendern des finalen Zustands
6. MQTT-State-Feedback an `wordclock/state`

### Aktuelle Optimierungen

- **Clock-Morph-Skip:** Identische Wortmasken lösen keinen erneuten Morph aus, sondern ein direktes Re-Render.
- **Clock-Morph-Cap:** Morph-Dauer ist auf `CLOCK_MORPH_MAX_MS = 1200 ms` begrenzt, unabhängig vom konfigurierten `transition`-Wert. Verhindert Loop-Blockaden bei hohen Transition-Einstellungen.
- **MQTT JSON Coalescing:** Rapid-Fire-Updates auf `wordclock/set` werden innerhalb von 60 ms zusammengeführt. Nur der finale Zustand wird angewendet und publiziert.
- **Debounced Preferences Save:** `StateManager::scheduleSave()` entkoppelt NVS-Schreiboperationen vom MQTT-Callback-Pfad. Schreiben erfolgt frühestens 750 ms nach dem letzten Befehl.

---

## Effekte

| Name | Beschreibung |
|---|---|
| `clock` | Wortuhr mit Morph-Animation |
| `wifi` | WLAN-Ring-Animation (Startup) |
| `waterdrop` | Wassertropfen-Simulation |
| `love` | Herz-Animation |
| `colorloop` | Farbkreislauf |
| `colorwipe` | Farbwischer |
| `fire2d` | 2D-Feuer |
| `matrix` | Matrix-Regen |
| `plasma` | Plasma-Wellen |
| `inward` | Inward-Ripple |
| `twinkle` | Funkeln |
| `balls` | Hüpfende Bälle |
| `aurora` | Aurora-Borealis |
| `enchantment` | Verzauberungseffekt |
| `snake` | Snake |

---

## Home Assistant / MQTT

### Haupttopics

| Topic | Richtung | Beschreibung |
|---|---|---|
| `wordclock/set` | → Gerät | JSON-Steuerung: `state`, `brightness`, `effect`, `color` |
| `wordclock/state` | ← Gerät | Aktueller Light-State als JSON |

### Tuning-Topics

| Topic | Wertebereich | Beschreibung |
|---|---|---|
| `wordclock/speed/set` | 1–100 | Effektgeschwindigkeit |
| `wordclock/intensity/set` | 1–100 | Effektintensität |
| `wordclock/transition/set` | 200–10000 ms | Übergangsdauer (Clock-Morph intern auf 1200 ms gecappt) |
| `wordclock/palette/set` | 0–4 | Farbpalette: `Auto`, `Warm`, `Cool`, `Natur`, `Candy` |
| `wordclock/hueshift/set` | 0–359 | Farbton-Verschiebung |

Jedes `*/set`-Topic hat ein korrespondierendes `*/state`-Topic.

### Service-Topics

| Topic | Payload | Aktion |
|---|---|---|
| `wordclock/tuning_reset/set` | `RESET` | Tuning auf Defaults zurücksetzen |
| `wordclock/service/set` | `default` / `tuning_default` / `reset_tuning` | Gerätezustand zurücksetzen |

### Home Assistant Discovery

Discovery wird automatisch beim Connect publiziert für:
- Light Entity
- Tuning-Entities (Speed, Intensity, Transition, Palette, HueShift)
- Diagnose-Sensoren

---

## Wertebereiche (`ControlConfig.h`)

| Parameter | Min | Max | Default |
|---|---|---|---|
| Brightness | 0 | 255 | 128 |
| Speed | 1 | 100 | 50 |
| Intensity | 1 | 100 | 50 |
| Transition | 200 ms | 10000 ms | 800 ms |
| Palette | 0 | 4 | 0 (Auto) |
| HueShift | 0 | 359 | 0 |

---

## Web-UI & SPIFFS

- HTML/CSS/JS-Seiten sind in `src/web_pages.cpp` als String-Literale eingebettet.
- Externe Assets werden per HTTP aus `data/` über SPIFFS ausgeliefert.
- Aktive SPIFFS-Assets: `data/Pfote.png`, `data/ziffernblatt.svg`
- Filesystem separat flashen: `pio run -t uploadfs`

---

## Serial Commands

Verbindung: COM5, 115200 baud

```text
Help                              Befehlsübersicht
Status                            Gerätestatus
Diag                              Diagnoseinformationen
RTC                               RTC-Status und Zeit
Creds Show                        WLAN/MQTT-Credentials anzeigen

Effect Set <name>                 Effekt setzen
Brightness Set <0-255>            Helligkeit setzen
Color Set <r> <g> <b>             Farbe setzen (0-255 je Kanal)
Speed Set <1-100>                 Geschwindigkeit setzen
Intensity Set <1-100>             Intensität setzen
Transition Set <200-10000>        Übergangsdauer setzen

Debug Help                        Debug-Kategorie-Übersicht
Debug Status                      Aktive Kategorien anzeigen
Debug All On                      Alle Kategorien aktivieren
Debug <kategorie> On/Off          Einzelne Kategorie schalten

Test Smoke                        Schnelltest (Boot-Validierung)
Test Self                         Vollständiger Selbsttest
Test Morph <HH:MM>                Clock-Morph zu gegebener Zeit testen
```

Legacy-Kurzformen (`effect`, `speed`, `intensity`, `transition`, `debug`) funktionieren weiterhin.

---

## Debug-Kategorien

Alle Kategorien sind zur Laufzeit per Serial ein- und ausschaltbar:

`Boot` · `Main` · `Loop` · `MQTT` · `WiFi` · `RTC` · `Time` · `State` · `EffectManager` · `Effects` · `LEDMatrix` · `Memory` · `OTA` · `Serial` · `Test`

---

## Sicherheitshinweise

- OTA-Passwort in `platformio.ini` vor Produktiveinsatz ändern
- MQTT-Credentials nicht im Klartext weitergeben
- WLAN- und Broker-Konfiguration läuft über das Setup-Portal und wird in NVS gespeichert
- Auf Windows: vor Upload sicherstellen, dass kein Serial-Monitor auf COM5 aktiv ist

---

## GitHub OTA Auto-Update

Die Firmware prueft automatisch auf neue Versionen in GitHub und installiert diese selbststaendig.

### Wie es funktioniert

1. Beim Start wartet das Geraet 2 Minuten.
2. Danach wird `ota_manifest.json` von GitHub geladen.
3. `version` im Manifest wird mit der lokal kompilierten `FIRMWARE_VERSION` verglichen.
4. Wenn `remote > local`, startet HTTPS-OTA mit `firmware_url`.
5. Danach wird alle 6 Stunden erneut geprueft.

Manifest-Datei im Repo-Root:

```json
{
    "version": "0.1.0",
    "firmware_url": "https://github.com/userkai14122001/WordClockV2/releases/latest/download/firmware.bin"
}
```

### Release-Ablauf

1. Neue Firmware bauen:
     `pio run`
2. Firmware-Datei aus `.pio/build/seeed_xiao_esp32c3_usb/firmware.bin` nehmen.
3. In GitHub einen Release erstellen und die Datei als Asset exakt `firmware.bin` hochladen.
4. In `ota_manifest.json` die `version` erhoehen (z. B. `0.1.1`) und commit/push.
5. In `platformio.ini` `FIRMWARE_VERSION` auf denselben Wert setzen, commit/push.

### Manueller OTA-Check

Per Serial kann ein sofortiger Check gestartet werden:

`Update Check`

Legacy-Alias:

`ota`

---

## Hinweise

- Bei `transition`-Werten > 1200 ms wird der Clock-Morph intern gecappt. Der konfigurierte Wert bleibt gespeichert und gilt uneingeschränkt für Effektwechsel.
- SPIFFS und Firmware sind getrennte Flash-Partitionen und müssen separat geflasht werden.
- NTP-Sync erfordert eine aktive WLAN-Verbindung. Ohne NTP läuft die RTC autonom weiter.
