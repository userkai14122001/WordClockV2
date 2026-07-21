#!/usr/bin/env python3
"""
WordClock USB Flasher - Standalone executable
Reliable ESP32-C3 flashing via esptool backend.
Default mode flashes mini_updater.bin and lets device self-update via OTA.
"""

import sys
import json
import urllib.request
import urllib.error
from pathlib import Path

try:
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not found. Install with: pip install pyserial")
    sys.exit(1)

try:
    import esptool
except ImportError:
    print("ERROR: esptool not found. Install with: pip install esptool")
    sys.exit(1)


RELEASE_API_URL = "https://api.github.com/repos/userkai14122001/WordClockV2/releases/latest"
RAW_FIRMWARE_BASE_URL = "https://raw.githubusercontent.com/userkai14122001/WordClockV2/main/firmware/"
MINI_UPDATER_FILENAME = "mini_updater.bin"
FULL_FIRMWARE_FILENAME = "firmware.bin"


def parse_mode_from_args() -> bool:
    args = [arg.strip().lower() for arg in sys.argv[1:]]
    return "--full" in args


def _download_url_to_path(url: str, target: Path, timeout_sec: int = 60) -> bool:
    req = urllib.request.Request(url, headers={"User-Agent": "WordClock-Flasher"})
    with urllib.request.urlopen(req, timeout=timeout_sec) as response:
        data = response.read()
    if not data:
        return False
    target.write_bytes(data)
    return True


def download_latest_asset(target: Path, asset_name: str) -> bool:
    try:
        print(f"[*] {asset_name} nicht gefunden, lade aktuelle Datei aus dem Repo...")
        req = urllib.request.Request(
            RELEASE_API_URL,
            headers={"User-Agent": "WordClock-Flasher"},
        )
        with urllib.request.urlopen(req, timeout=20) as response:
            release = json.loads(response.read().decode("utf-8"))

        assets = release.get("assets", [])
        asset = next((a for a in assets if a.get("name") == asset_name), None)
        if asset:
            download_url = asset.get("browser_download_url")
            if download_url and _download_url_to_path(download_url, target):
                tag = release.get("tag_name", "unknown")
                print(f"[+] Datei geladen: {target} ({target.stat().st_size} bytes) aus Release {tag}")
                return True

        raw_url = RAW_FIRMWARE_BASE_URL + asset_name
        print(f"[*] Release-Asset fehlt, versuche Raw-Download: {raw_url}")
        if _download_url_to_path(raw_url, target):
            print(f"[+] Datei geladen: {target} ({target.stat().st_size} bytes) aus Raw-Repo")
            return True

        print(f"[!] ERROR: {asset_name} wurde weder im neuesten Release noch im Raw-Repo gefunden.")
        return False
    except urllib.error.HTTPError as ex:
        print(f"[!] ERROR beim Download ({asset_name}): HTTP {ex.code}")
        return False
    except Exception as ex:
        print(f"[!] ERROR beim Download von {asset_name}: {ex}")
        return False


def find_esp_ports():
    ports = serial.tools.list_ports.comports()
    matches = []
    for port in ports:
        desc = (port.description or "").lower()
        hwid = (port.hwid or "").lower()
        if any(token in desc for token in ["ch340", "cp210", "esp32", "usb serial", "uart"]):
            matches.append(port.device)
            continue
        if any(token in hwid for token in ["1a86", "10c4", "303a"]):
            matches.append(port.device)
    return matches


def flash_with_esptool(port: str, firmware: Path):
    args = [
        "--chip",
        "esp32c3",
        "--port",
        port,
        "--baud",
        "460800",
        "--before",
        "default-reset",
        "--after",
        "hard-reset",
        "--no-stub",
        "write-flash",
        "-z",
        "--flash-mode",
        "keep",
        "--flash-freq",
        "keep",
        "--flash-size",
        "keep",
        "0x10000",
        str(firmware),
        "0x190000",
        str(firmware),
    ]
    esptool.main(args)


def pause_and_exit(code: int):
    try:
        input("\nDruecke Enter zum Schliessen...")
    except EOFError:
        pass
    sys.exit(code)


def main():
    print("=" * 60)
    print("  WordClock USB Flasher")
    print("=" * 60)

    full_mode = parse_mode_from_args()
    firmware_name = FULL_FIRMWARE_FILENAME if full_mode else MINI_UPDATER_FILENAME
    firmware_file = Path(firmware_name)

    if full_mode:
        print("[*] Modus: FULL firmware (override per --full)")
    else:
        print("[*] Modus: MINI updater (default)")

    if not firmware_file.exists():
        if not download_latest_asset(firmware_file, firmware_name):
            print(f"[!] ERROR: {firmware_file} weiterhin nicht verfuegbar.")
            if not full_mode:
                print("[*] Hinweis: Mini-Updater zuerst bauen und als mini_updater.bin bereitstellen")
                print("[*] Build (PlatformIO): pio run -e seeed_xiao_esp32c3_mini_updater")
            pause_and_exit(1)

    print(f"[+] Found firmware: {firmware_file} ({firmware_file.stat().st_size} bytes)")

    ports = find_esp_ports()
    if not ports:
        print("[!] ERROR: No ESP32 device found")
        print("[*] Check USB data cable, driver, and COM port availability")
        pause_and_exit(1)

    port = ports[0]
    if len(ports) > 1:
        print(f"[!] Multiple serial ports detected: {ports}")
        print(f"[*] Using first match: {port}")
    else:
        print(f"[+] Device found on: {port}")

    try:
        print("[*] Starting flash with esptool...")
        flash_with_esptool(port, firmware_file)
        print("\n[+] SUCCESS: Flash finished. Device rebooting...")
        pause_and_exit(0)
    except SystemExit as ex:
        code = int(ex.code) if isinstance(ex.code, int) else 1
        if code == 0:
            raise
        print("\n[!] Flash failed.")
        print("[*] If port is busy, close serial monitors and try again.")
        print("[*] If connect fails, hold BOOT while starting Flash.exe.")
        pause_and_exit(code)
    except Exception as ex:
        print(f"\n[!] ERROR: {ex}")
        print("[*] Close all COM-port tools and retry.")
        pause_and_exit(1)


if __name__ == "__main__":
    main()
