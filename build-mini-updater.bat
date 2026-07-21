@echo off
setlocal EnableExtensions

REM Build and stage the mini updater binary for Flash.exe usage.

echo.
echo =========================================
echo  WordClock Mini Updater Builder
echo =========================================
echo.

set "PYTHON_EXE=python"
%PYTHON_EXE% --version >nul 2>&1
if errorlevel 1 (
    if exist "%LocalAppData%\Programs\Python\Python312\python.exe" (
        set "PYTHON_EXE=%LocalAppData%\Programs\Python\Python312\python.exe"
    ) else (
        echo [ERROR] Python not found. Install from python.org
        pause
        exit /b 1
    )
)

echo [*] Installing build tooling...
"%PYTHON_EXE%" -m pip install --quiet platformio
if errorlevel 1 (
    echo [ERROR] Failed to install PlatformIO
    pause
    exit /b 1
)

echo [*] Building mini updater firmware...
"%PYTHON_EXE%" -m platformio run -e seeed_xiao_esp32c3_mini_updater
if errorlevel 1 (
    echo [ERROR] Mini updater build failed
    pause
    exit /b 1
)

set "SRC=.pio\build\seeed_xiao_esp32c3_mini_updater\firmware.bin"
if not exist "%SRC%" (
    echo [ERROR] Build artifact not found: %SRC%
    pause
    exit /b 1
)

if not exist "firmware" mkdir "firmware"
if not exist "dist" mkdir "dist"

copy /Y "%SRC%" "mini_updater.bin" >nul
copy /Y "%SRC%" "firmware\mini_updater.bin" >nul
copy /Y "%SRC%" "dist\mini_updater.bin" >nul

echo [OK] mini_updater.bin created:
echo      - mini_updater.bin
echo      - firmware\mini_updater.bin
echo      - dist\mini_updater.bin
echo.
echo Next step:
echo   Run dist\Flash.exe (default MINI mode)
pause
