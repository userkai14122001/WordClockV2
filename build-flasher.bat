@echo off
REM Build Flash.exe from flash_wordclock.py
REM Requirements: pip install pyinstaller pyserial

echo.
echo =========================================
echo  WordClock Flasher EXE Builder
echo =========================================
echo.

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python not found. Install from python.org
    pause
    exit /b 1
)

echo [*] Installing dependencies...
pip install --quiet pyinstaller pyserial

if not exist flash_wordclock.py (
    echo [ERROR] flash_wordclock.py not found
    pause
    exit /b 1
)

echo [*] Building Flash.exe...
pyinstaller --onefile ^
    --name Flash ^
    --icon=icon.ico ^
    --console ^
    --add-data "firmware.bin;." ^
    flash_wordclock.py

if exist dist\Flash.exe (
    echo [OK] Build successful!
    echo [*] Flash.exe created in dist\ folder
    echo.
    echo To use:
    echo   1. Copy Flash.exe to a folder with firmware.bin
    echo   2. Run: Flash.exe
    pause
) else (
    echo [ERROR] Build failed
    pause
    exit /b 1
)
