# WordClock

ESP32-C3-basierte Wortuhr mit 11×10 NeoPixel-Matrix, DS3231-RTC, WiFi-Setup-Portal, MQTT/Home-Assistant-Integration, Zeitschaltung-Automation und mehreren Animationseffekten.

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
│   └── ziffernblatt.svg
├── include/
│   ├── config.h
│   ├── ControlConfig.h
│   ├── DebugManager.h
│   ├── EffectManager.h
│   ├── effects.h
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
│   ├── WiFiManager.h
│   ├── WordClockLayout.h
│   ├── WordClockLayoutPresets.h
│   └── ZeitschaltungManager.h
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
    ├── WordClockLayout.cpp
    ├── WordClockLayoutPresets.cpp
    ├── ZeitschaltungManager.cpp
    └── effects/
        ├── AuroraEffect.cpp
        ├── BouncingBallsEffect.cpp
        ├── ColorloopEffect.cpp
        ├── ColorwipeEffect.cpp
        ├── effect_helpers.h
        ├── EnchantmentEffect.cpp
        ├── Fire2DEffect.cpp
        ├── GreenRingWaveEffect.cpp
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

`platformio.ini` enthält vier Environments:

| Environment | Zweck |
|---|---|
| `seeed_xiao_esp32c3_usb` | USB-Upload (Standard) |
| `seeed_xiao_esp32c3_ota` | OTA-Upload über WLAN (stable) |
| `seeed_xiao_esp32c3_usb_beta` | USB-Upload für Beta-Testing |
| `seeed_xiao_esp32c3_ota_beta` | OTA-Upload für Beta-Testing |

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
| `ZeitschaltungManager` | Zeitbasierte Regeln (Power, Helligkeit, Effekt) |
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

- **Clock-Morph-Entkopplung:** Clock-Minute-Morph läuft mit fester kurzer Dauer (220 ms), unabhängig vom `transition`-Parameter. Dies verhindert MQTT/Steuer-Blockaden.
- **Transition nur für Effekte:** Der `transition`-Parameter (`uebergang`) steuert ausschließlich Effektwechsel-Animationen, nicht die Uhrzeit-Übergänge.
- **Clock-Morph-Skip:** Identische Wortmasken lösen keinen erneuten Morph aus, sondern ein direktes Re-Render.
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
| `wordclock/transition/set` | 200–10000 ms | Effektwechsel-Animationsdauer (beeinflusst Uhrzeit-Morph nicht) |
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
- Aktive SPIFFS-Assets: `data/ziffernblatt.svg`
- Filesystem separat flashen: `pio run -t uploadfs`
- Studio-Ansicht pollt mit 45 Hz Zielrate (`/api/live-status`, Intervall 22 ms).
- API-Endpunkte: `/api/status` (voll), `/api/status-lite` (leicht), `/api/live-status` (Studio optimiert).
- Legacy-Route `/layout` ist ein Redirect auf `/main`.

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
OTA Info                           OTA-Kanal/Profil/Version anzeigen
OTA Check                          Sofortigen OTA-Check starten
Zeitschaltung list                 Alle Regeln anzeigen
Zeitschaltung rename <idx> <name>  Regel umbenennen (idx: 0-4)
Zeitschaltung set <idx> <HH> <MM> <on|off> [brightness] [effect]
Zeitschaltung enable <idx>         Regel aktivieren
Zeitschaltung disable <idx>        Regel deaktivieren
Zeitschaltung clear                Alle Regeln löschen
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

Die Firmware prüft automatisch auf neue Versionen in GitHub und installiert diese selbstständig.

### Wie es funktioniert

1. Nach dem Start wartet das Gerät 30 Sekunden.
2. Danach wird `ota_manifest.json` von GitHub (stable) oder lokales OTA_CHANNEL (beta) geladen.
3. `version` im Manifest wird mit der lokal kompilierten `FIRMWARE_VERSION` verglichen.
4. Wenn `remote > local`, startet HTTPS-OTA mit `firmware_url`.
5. Nach einem erfolgreichen Update werden Checks regelmäßig wiederholt (Standard: täglich).

### Manifest-Format

```json
{
    "channels": {
        "stable": {
            "version": "0.3.8",
            "firmware_url": "https://github.com/userkai14122001/WordClockV2/releases/download/v0.3.8/firmware.bin",
            "sha256": "...",
            "status": "ready"
        },
        "beta": {
            "version": "0.2.44",
            "firmware_url": "https://github.com/userkai14122001/WordClockV2/releases/download/v0.2.44/firmware.bin",
            "sha256": "...",
            "status": "hold"
        }
    }
}
```

### OTA-Kanäle

- **stable:** Regulärer OTA-Kanal für alle Uhren (Standardeinstellung).
- **beta:** Beta-Kanal für Testuhr (compile `-DOTA_CHANNEL=\"beta\"`).

### Release-Workflow (empfohlen)

1. Firmware-Version in `platformio.ini` anheben.
2. Commit + Push auf `main`.
3. Git-Tag `vX.Y.Z` erstellen und pushen.
4. GitHub Actions erstellt den Release, lädt `firmware.bin` hoch und aktualisiert `ota_manifest.json` automatisch.

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
