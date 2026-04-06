#include "rtc_module.hpp"
#include "DebugManager.h"
#include <time.h>

static RTC_DS3231 g_rtc;
static bool g_rtc_available = false;

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

bool rtc_init(uint8_t sda_pin, uint8_t scl_pin) {
    Wire.setPins((int)sda_pin, (int)scl_pin);
    Wire.begin((int)sda_pin, (int)scl_pin);
    Wire.setClock(100000);

    g_rtc_available = g_rtc.begin(&Wire);
    if (!g_rtc_available) {
        waitMs(3000);
        Wire.setPins((int)sda_pin, (int)scl_pin);
        Wire.begin((int)sda_pin, (int)scl_pin);
        Wire.setClock(100000);
        g_rtc_available = g_rtc.begin(&Wire);
    }

    if (g_rtc_available && g_rtc.lostPower()) {
        // RTC hat Strom verloren – keine Compile-Zeit setzen (wäre falsch).
        // Zeit wird beim nächsten NTP-Sync automatisch korrigiert.
        DebugManager::println(DebugCategory::RTC, "rtc_module: RTC hat Strom verloren - warte auf NTP-Sync.");
    }

    // Germany: CET/CEST including automatic DST switching
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    return g_rtc_available;
}

void rtc_set_time_utc(const DateTime& dt_utc) {
    if (!g_rtc_available) return;
    g_rtc.adjust(dt_utc);
}

DateTime rtc_get_time_local() {
    if (!g_rtc_available) {
        return DateTime((uint32_t)0);
    }

    // RTC stores UTC
    DateTime now_utc = g_rtc.now();
    time_t t = now_utc.unixtime();

    // UTC -> local via TZ
    struct tm local_tm;
    localtime_r(&t, &local_tm);

    return DateTime(
        local_tm.tm_year + 1900,
        local_tm.tm_mon + 1,
        local_tm.tm_mday,
        local_tm.tm_hour,
        local_tm.tm_min,
        local_tm.tm_sec
    );
}

bool rtc_is_available() {
    return g_rtc_available;
}

bool rtc_read_oscillator_stop_flag() {
    if (!g_rtc_available) {
        return true;
    }

    Wire.beginTransmission(0x68);
    Wire.write(0x0F);
    if (Wire.endTransmission(false) != 0) {
        return true;
    }
    if (Wire.requestFrom(0x68, 1) != 1) {
        return true;
    }

    const uint8_t statusReg = Wire.read();
    return (statusReg & 0x80) != 0;
}

float rtc_get_temperature_c() {
    if (!g_rtc_available) {
        return NAN;
    }
    return g_rtc.getTemperature();
}