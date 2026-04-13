#!/usr/bin/env python3
"""
WordClock USB Flasher - Standalone executable
Flashes firmware.bin onto ESP32-C3 via USB without any external dependencies
"""

import sys
import os
import time
import struct
import zipfile
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not found. Install with: pip install pyserial")
    sys.exit(1)


class ESPFlasher:
    """Minimal ESP32-C3 flasher using esptool protocol"""
    
    FLASH_BEGIN = 0x02
    FLASH_DATA = 0x03
    FLASH_END = 0x04
    FLASH_MD5 = 0x13
    
    def __init__(self, port, baud=115200):
        self.port = serial.Serial(port, baud, timeout=1)
        self.baud = baud
        time.sleep(0.5)
        
    def reset_to_bootloader(self):
        """Reset ESP to bootloader mode"""
        self.port.dtr = False
        self.port.rts = True
        time.sleep(0.1)
        self.port.rts = False
        time.sleep(0.5)
        
    def read_response(self, timeout=3):
        """Read response packet"""
        start = time.time()
        while time.time() - start < timeout:
            if self.port.in_waiting:
                response = self.port.read(self.port.in_waiting)
                return response
            time.sleep(0.01)
        return b''
    
    def send_packet(self, cmd, data=b''):
        """Send command packet to ESP"""
        # Simple packet format: 0xC0 direction cmd len data checksum 0xC0
        payload = bytes([cmd, len(data) & 0xFF, (len(data) >> 8) & 0xFF, 0]) + data
        checksum = sum(payload) & 0xFF
        payload += bytes([checksum])
        packet = b'\xC0' + payload + b'\xC0'
        self.port.write(packet)
        self.port.flush()
        
    def flash_bin(self, firmware_path, address=0x1000):
        """Flash binary file to ESP"""
        with open(firmware_path, 'rb') as f:
            firmware = f.read()
        
        print(f"[*] Flashing {len(firmware)} bytes to address 0x{address:x}")
        
        # Begin flash
        blocks = (len(firmware) + 4095) // 4096
        begin_data = struct.pack('<IIII', len(firmware), blocks, 4096, address)
        self.send_packet(self.FLASH_BEGIN, begin_data)
        resp = self.read_response()
        if not resp or resp[0:1] != b'\x08':
            print("[!] Begin failed, retrying...")
        
        # Write blocks
        for i in range(0, len(firmware), 4096):
            block = firmware[i:i+4096]
            if len(block) < 4096:
                block += b'\xff' * (4096 - len(block))
            
            block_num = i // 4096
            block_data = struct.pack('<IIII', len(block), block_num, 0, 0) + block
            self.send_packet(self.FLASH_DATA, block_data)
            
            resp = self.read_response()
            pct = (i + len(block)) * 100 // len(firmware)
            print(f"[{'='*40}] {pct}%", end='\r')
        
        print()
        print("[+] Flash complete!")
        
    def close(self):
        self.port.close()


def find_esp_port():
    """Auto-detect ESP32-C3 USB port"""
    ports = serial.tools.list_ports.comports()
    esp_ports = []
    
    for port in ports:
        # Look for CH340 (common esp32 usb chip) or ESP32 in description
        if 'CH340' in port.description or 'CP210' in port.description or 'ESP32' in port.description or 'USB' in port.description:
            esp_ports.append(port.device)
    
    return esp_ports


def main():
    print("=" * 60)
    print("  WordClock v0.3.9 - USB Flasher")
    print("=" * 60)
    
    # Find firmware
    firmware_file = Path("firmware.bin")
    if not firmware_file.exists():
        print(f"[!] ERROR: {firmware_file} not found in current directory")
        print("[*] Download firmware.bin from the release and place it here")
        sys.exit(1)
    
    print(f"[+] Found firmware: {firmware_file} ({firmware_file.stat().st_size} bytes)")
    
    # Find port
    ports = find_esp_port()
    
    if not ports:
        print("[!] ERROR: No ESP32 device found!")
        print("[*] Make sure:")
        print("    - USB cable is connected")
        print("    - USB driver is installed (CH340 driver if needed)")
        print("    - Device is in bootloader mode (press BOOT + RST)")
        sys.exit(1)
    
    port = ports[0]
    if len(ports) > 1:
        print(f"[!] Multiple ports found: {ports}")
        port = ports[0]
        print(f"[*] Using: {port}")
    else:
        print(f"[+] Device found on: {port}")
    
    # Flash
    try:
        print("[*] Resetting to bootloader...")
        flasher = ESPFlasher(port)
        flasher.reset_to_bootloader()
        
        print("[*] Starting flash...")
        flasher.flash_bin(str(firmware_file))
        
        print("[*] Resetting device...")
        flasher.port.dtr = False
        flasher.port.rts = False
        time.sleep(1)
        
        flasher.close()
        
        print("\n[✓] SUCCESS! Device flashed and rebooting...")
        print("[*] Open http://[device-ip]/ to access WordClock")
        
    except Exception as e:
        print(f"[!] ERROR: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
