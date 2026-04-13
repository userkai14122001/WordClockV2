# WordClock USB Flasher - Benutzerhandbuch

## Schnelleinstieg

1. **Download**: Laden Sie auf der [Release-Seite](https://github.com/userkai14122001/WordClockV2/releases) die Dateien herunter:
   - `Flash.exe` (Windows) oder `Flash` (macOS/Linux)
   - `firmware.bin`

2. **Ordner erstellen**: Legen Sie beide Dateien in denselben Ordner

3. **Device anschließen**: Verbinden Sie den ESP32-C3 über USB mit dem Computer

4. **Flash**: Doppelklick auf `Flash.exe` (oder starten Sie `Flash` von der Kommandozeile)

5. **Fertig**: Nach ~30 Sekunden startet das Gerät neu und verbindet sich mit dem WLAN

---

## Was wird benötigt?

- **Windows**: Flash.exe (das ist alles!)
- **macOS/Linux**: Flash Binary (vorgebaut)
- Ein **USB-Kabel** (nicht einfach Power!)
- Ein **Internet-verbundener Computer**

## Keine Treiber nötig?

✓ Moderne Windows/macOS/Linux: USB-Erkennung funktioniert out-of-the-box
✓ Falls nicht: Installieren Sie den [CH340-Treiber](https://wiki.wemos.cc/downloads) (für ältere Windows)

---

## Fehlerbehebung

### "Kein Device gefunden"
- Kabel prüfen (echtes USB-Datenkabel, nicht nur Power!)
- Gerät neu anstecken
- **Boot-Taste halten** während des Ansteckens (in Setup-Mode gehen)
- Treiber: `CH340` bei Windows installieren

### "COM-Port wird nicht erkannt"
- Gerät-Manager öffnen → Nach "USB Serial Device" oder "CP210x" suchen
- Wenn vorhanden, aber kein COM-Port: CH340-Treiber installieren
- Windows SDK neu installieren (enthält USB-Treiber)

### Flash hängt
- Gerät trennen, 5s warten, Flash neu starten
- Versuch 2-3x, manchmal ist der USB-Timing flaky

---

## Lokales Bauen

Die Flasher-EXE selbst bauen:

```bash
# Voraussetzung: Python 3.11+ installiert

# 1. Dependencies
pip install pyinstaller pyserial

# 2. EXE bauen
pyinstaller --onefile flash_wordclock.py

# 3. firmware.bin in denselben Ordner kopieren
cp .pio/build/seeed_xiao_esp32c3_ota/firmware.bin .

# 4. Fertig!
# dist/Flash.exe kann jetzt verteilt werden
```

---

## Entwickler

Die Flasher-Komponenten:
- `flash_wordclock.py` - Python-Source für den Flasher
- `build-flasher.bat` - Lokal bauen (Windows)
- `.github/workflows/release-with-flasher.yml` - GitHub Actions (automatischer Build)

Bei Tag-Push (`v0.3.9`, etc.) erstellt GitHub Actions automatisch:
- `firmware.bin` (OTA-kompiliert)
- `Flash.exe` (Windows)
- `Flash` (macOS/Linux)

---

## Support

Problem-Meldungen im GitHub Issues: https://github.com/userkai14122001/WordClockV2/issues
