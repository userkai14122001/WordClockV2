#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include "RTCManager.h"

class TimeManager {
public:
    // Singleton
    static TimeManager& getInstance();
    
    // Initialize with RTCManager reference
    void init(RTCManager& rtc);
    
    // Time queries
    DateTime getTime() const;
    uint8_t getHour() const { return getTime().hour(); }
    uint8_t getMinute() const { return getTime().minute(); }
    uint8_t getSecond() const { return getTime().second(); }
    uint16_t getDayOfYear();
    
    // NTP Synchronization (requires WiFi)
    void syncNTP(const char* ntp_server = "pool.ntp.org");
    bool isSyncedWithNTP() const { return ntp_synced; }
    unsigned long getLastNTPSync() const { return last_ntp_sync; }
    
    // Auto-sync NTP periodically (call from loop)
    void updateNTPSync();
    
    // Timezone
    void setTimezone(const char* tz_string);
    const char* getTimezone() const { return timezone_string; }
    
    // Time validation
    bool isValidTime() const;
    bool isRTCAvailable() const { return rtc_manager != nullptr; }
    
    // Seconds since last full minute
    uint8_t getSecondsSinceMinute() const { return getTime().second(); }
    
    // Check if minute changed (for clock updates)
    bool hasMinuteChanged();
    
    // Manual time setting
    void setTime(const DateTime& dt);
    void setTime(uint16_t year, uint8_t month, uint8_t day,
                 uint8_t hour, uint8_t minute, uint8_t second);
    
    // Debug
    void printTime() const;
    
private:
    TimeManager() = default;
    
    RTCManager* rtc_manager = nullptr;
    struct tm cached_time_info = {};
    bool ntp_synced = false;
    unsigned long last_ntp_sync = 0;
    unsigned long ntp_sync_interval = 3600000;  // 1 hour
    unsigned long last_ntp_attempt = 0;
    unsigned long ntp_retry_interval = 300000;  // 5 minutes
    
    const char* timezone_string = "CET-1CEST,M3.5.0,M10.5.0/3";  // Central European Time
    
    int last_minute = -1;
};

#endif
