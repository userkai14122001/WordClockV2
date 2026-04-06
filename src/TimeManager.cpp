#include "TimeManager.h"
#include "DebugManager.h"
#include <time.h>
#include <WiFi.h>

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

TimeManager& TimeManager::getInstance() {
    static TimeManager instance;
    return instance;
}

void TimeManager::init(RTCManager& rtc) {
    rtc_manager = &rtc;
    // NTP sync happens later in setup(), after WiFi is connected.
    DebugManager::println(DebugCategory::Time, "TimeManager: Initialized");
}

DateTime TimeManager::getTime() const {
    if (rtc_manager != nullptr) {
        return rtc_manager->getTime();
    }
    
    // Fallback to system time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    return DateTime(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

uint16_t TimeManager::getDayOfYear() {
    DateTime now = getTime();
    
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Check for leap year
    bool is_leap = (now.year() % 4 == 0 && now.year() % 100 != 0) || (now.year() % 400 == 0);
    
    uint16_t day_of_year = now.day();
    for (uint8_t i = 0; i < now.month() - 1; i++) {
        day_of_year += days_in_month[i];
        if (i == 1 && is_leap) {
            day_of_year++;
        }
    }
    
    return day_of_year;
}

void TimeManager::syncNTP(const char* ntp_server) {
    DebugManager::print(DebugCategory::Time, "TimeManager: Syncing with NTP (");
    DebugManager::print(DebugCategory::Time, ntp_server);
    DebugManager::println(DebugCategory::Time, ")...");
    
    // Set timezone first
    configTzTime(timezone_string, ntp_server);
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Wait for NTP sync (max 30 seconds)
    int attempts = 0;
    while (timeinfo->tm_year < (2020 - 1900) && attempts < 60) {
        waitMs(500);
        now = time(nullptr);
        timeinfo = localtime(&now);
        attempts++;
    }
    
    if (timeinfo->tm_year >= (2020 - 1900)) {
        ntp_synced = true;
        last_ntp_sync = millis();
        
        // Update RTC with NTP time
        if (rtc_manager != nullptr && rtc_manager->isAvailable()) {
            DateTime ntp_time(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            rtc_manager->setTime(ntp_time);
            DebugManager::println(DebugCategory::Time, "TimeManager: RTC synced with NTP");
        } else {
            DebugManager::println(DebugCategory::Time, "TimeManager: RTC unavailable, using NTP system time only");
        }
        
        DebugManager::print(DebugCategory::Time, "TimeManager: NTP sync successful - ");
        printTime();
    } else {
        ntp_synced = false;
        DebugManager::println(DebugCategory::Time, "TimeManager: NTP sync failed - using RTC");
    }
}

void TimeManager::updateNTPSync() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();
    
    // First attempt immediately after boot, then retry every 5 minutes.
    if (!ntp_synced && (last_ntp_attempt == 0 || (now - last_ntp_attempt > ntp_retry_interval))) {
        last_ntp_attempt = now;
        syncNTP();
        return;
    }
    
    // If synced, re-sync every hour
    if (ntp_synced && (now - last_ntp_sync > ntp_sync_interval)) {
        syncNTP();
    }
}

void TimeManager::setTimezone(const char* tz_string) {
    timezone_string = tz_string;
    configTzTime(tz_string, "pool.ntp.org");
    DebugManager::print(DebugCategory::Time, "TimeManager: Timezone set to ");
    DebugManager::println(DebugCategory::Time, tz_string);
}

bool TimeManager::isValidTime() const {
    if (rtc_manager == nullptr) {
        return false;
    }
    
    DateTime now = rtc_manager->getTime();
    return now.year() >= 2020;
}

bool TimeManager::hasMinuteChanged() {
    int current_minute = getTime().minute();
    
    if (current_minute != last_minute) {
        last_minute = current_minute;
        return true;
    }
    
    return false;
}

void TimeManager::setTime(const DateTime& dt) {
    if (rtc_manager != nullptr) {
        rtc_manager->setTime(dt);
        DebugManager::print(DebugCategory::Time, "TimeManager: Time set to ");
        printTime();
    }
}

void TimeManager::setTime(uint16_t year, uint8_t month, uint8_t day,
                         uint8_t hour, uint8_t minute, uint8_t second) {
    setTime(DateTime(year, month, day, hour, minute, second));
}

void TimeManager::printTime() const {
    DateTime now = getTime();
    DebugManager::printf(DebugCategory::Time, "TimeManager: %04d-%02d-%02d %02d:%02d:%02d\n",
                         now.year(), now.month(), now.day(),
                         now.hour(), now.minute(), now.second());
}
