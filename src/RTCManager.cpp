#include "RTCManager.h"
#include "DebugManager.h"
#include "rtc_module.hpp"
#include <time.h>
#include <Wire.h>

static constexpr uint16_t RTC_HEALTH_CHECK_MS = 30000;
static constexpr float RTC_TEMP_WARN_LOW_C = -20.0f;
static constexpr float RTC_TEMP_WARN_HIGH_C = 65.0f;

static void printI2CScan(uint8_t sda_pin, uint8_t scl_pin) {
    Wire.begin((int)sda_pin, (int)scl_pin);
    Wire.setClock(100000);

    uint8_t foundCount = 0;
    bool foundDs3231 = false;
    DebugManager::printf(DebugCategory::RTC, "RTCManager: I2C scan on SDA=%u SCL=%u\n", sda_pin, scl_pin);
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            foundCount++;
            DebugManager::printf(DebugCategory::RTC, "RTCManager: I2C device found at 0x%02X\n", addr);
            if (addr == 0x68) {
                foundDs3231 = true;
            }
        }
    }

    if (foundCount == 0) {
        DebugManager::println(DebugCategory::RTC, "RTCManager: I2C scan found no devices");
    }
    if (!foundDs3231) {
        DebugManager::println(DebugCategory::RTC, "RTCManager: DS3231 address 0x68 not found on I2C bus");
    }
}

RTCManager::RTCManager(uint8_t sda_pin, uint8_t scl_pin)
    : sda_pin(sda_pin), scl_pin(scl_pin), available(false), power_was_lost(false) {
}

static DateTime localToUtc(const DateTime& local_dt) {
    struct tm local_tm;
    local_tm.tm_year = local_dt.year() - 1900;
    local_tm.tm_mon = local_dt.month() - 1;
    local_tm.tm_mday = local_dt.day();
    local_tm.tm_hour = local_dt.hour();
    local_tm.tm_min = local_dt.minute();
    local_tm.tm_sec = local_dt.second();
    local_tm.tm_isdst = -1;

    time_t local_epoch = mktime(&local_tm);
    struct tm utc_tm;
    gmtime_r(&local_epoch, &utc_tm);

    return DateTime(
        utc_tm.tm_year + 1900,
        utc_tm.tm_mon + 1,
        utc_tm.tm_mday,
        utc_tm.tm_hour,
        utc_tm.tm_min,
        utc_tm.tm_sec
    );
}

void RTCManager::init() {
    DebugManager::println(DebugCategory::RTC, "RTCManager: Initialisiere DS3231...");
    bool triedFallbackPins = false;
    const uint8_t fallbackSda = 6;
    const uint8_t fallbackScl = 7;

    available = rtc_init(sda_pin, scl_pin);
    if (!available) {
        // Fallback for some ESP32-C3 boards/wiring variants.
        if (!(sda_pin == fallbackSda && scl_pin == fallbackScl)) {
            triedFallbackPins = true;
            DebugManager::printf(DebugCategory::RTC, "RTCManager: Retry with fallback pins SDA=%u SCL=%u\n", fallbackSda, fallbackScl);
            available = rtc_init(fallbackSda, fallbackScl);
            if (available) {
                sda_pin = fallbackSda;
                scl_pin = fallbackScl;
                DebugManager::println(DebugCategory::RTC, "RTCManager: RTC found on fallback pins");
            }
        }
    }
    if (!available) {
        DebugManager::println(DebugCategory::RTC, "RTCManager: DS3231 NICHT gefunden!");
        if (triedFallbackPins) {
            printI2CScan(fallbackSda, fallbackScl);
        }
        printI2CScan(sda_pin, scl_pin);
        available = false;
        return;
    }

    DebugManager::println(DebugCategory::RTC, "RTCManager: OK");
    DebugManager::printf(DebugCategory::RTC, "RTCManager: Using I2C SDA=%u SCL=%u\n", sda_pin, scl_pin);

    DateTime now = rtc_get_time_local();
    DebugManager::printf(DebugCategory::RTC,
                         "RTCManager: Aktuelle Zeit: %d-%d-%d %d:%d:%d\n",
                         now.year(), now.month(), now.day(),
                         now.hour(), now.minute(), now.second());

    updateHealth();
}

DateTime RTCManager::getTime() {
    if (available) {
        return rtc_get_time_local();
    }
    
    // Fallback auf millis() wenn RTC nicht verfügbar
    uint32_t seconds = millis() / 1000;
    return DateTime(2024, 1, 1, 0, 0, seconds);
}

uint8_t RTCManager::getHour() {
    return getTime().hour();
}

uint8_t RTCManager::getMinute() {
    return getTime().minute();
}

uint8_t RTCManager::getSecond() {
    return getTime().second();
}

void RTCManager::setTime(const DateTime& dt) {
    if (!available) {
        static unsigned long lastUnavailableLogMs = 0;
        unsigned long now = millis();
        if (now - lastUnavailableLogMs >= 60000UL) {
            DebugManager::println(DebugCategory::RTC, "RTCManager: RTC nicht verfuegbar, Zeit kann nicht gesetzt werden");
            lastUnavailableLogMs = now;
        }
        return;
    }
    
    // Persist UTC in RTC; callers provide local time.
    DateTime utc_dt = localToUtc(dt);
    rtc_set_time_utc(utc_dt);
    DebugManager::printf(DebugCategory::RTC,
                         "RTCManager: Zeit gesetzt auf %d-%d-%d %d:%d:%d\n",
                         dt.year(), dt.month(), dt.day(),
                         dt.hour(), dt.minute(), dt.second());
}

void RTCManager::setTime(uint16_t year, uint8_t month, uint8_t day,
                         uint8_t hour, uint8_t minute, uint8_t second) {
    setTime(DateTime(year, month, day, hour, minute, second));
}

bool RTCManager::hasLostPower() {
    return power_was_lost;
}

void RTCManager::updateHealth() {
    const unsigned long nowMs = millis();
    if (last_health_check_ms != 0 && (nowMs - last_health_check_ms) < RTC_HEALTH_CHECK_MS) {
        return;
    }
    last_health_check_ms = nowMs;

    if (!available) {
        battery_warning = true;
        oscillator_stop_flag = true;
        temperature_warning = true;
        temperature_c = NAN;
        return;
    }

    oscillator_stop_flag = rtc_read_oscillator_stop_flag();
    battery_warning = oscillator_stop_flag;

    temperature_c = rtc_get_temperature_c();
    if (isnan(temperature_c)) {
        temperature_warning = true;
    } else {
        temperature_warning = (temperature_c < RTC_TEMP_WARN_LOW_C) || (temperature_c > RTC_TEMP_WARN_HIGH_C);
    }

    static bool lastWarningState = false;
    const bool warningNow = hasHealthWarning();
    if (warningNow != lastWarningState) {
        if (warningNow) {
            DebugManager::print(DebugCategory::RTC, "RTCManager: WARNING - osf=");
            DebugManager::print(DebugCategory::RTC, oscillator_stop_flag ? "1" : "0");
            DebugManager::print(DebugCategory::RTC, " temp=");
            if (isnan(temperature_c)) {
                DebugManager::println(DebugCategory::RTC, "nan");
            } else {
                DebugManager::println(DebugCategory::RTC, temperature_c, 2);
            }
        } else {
            DebugManager::println(DebugCategory::RTC, "RTCManager: RTC health back to normal");
        }
        lastWarningState = warningNow;
    }
}

void RTCManager::syncFromExternal(time_t unix_time) {
    if (!available) {
        return;
    }

    DateTime utc_dt((uint32_t)unix_time);
    rtc_set_time_utc(utc_dt);
    DebugManager::println(DebugCategory::RTC, "RTCManager: Externe Zeitsynchronisation");
}

void RTCManager::syncFromExternal(const DateTime& dt) {
    setTime(dt);
}
