#ifndef RTCMANAGER_CLASS_H
#define RTCMANAGER_CLASS_H

#include <Arduino.h>
#include <RTClib.h>

class RTCManager {
public:
    RTCManager(uint8_t sda_pin = GPIO_NUM_6, uint8_t scl_pin = GPIO_NUM_7);
    
    // Initialisierung
    void init();
    
    // Zeitabfragen
    DateTime getTime();
    uint8_t getHour();
    uint8_t getMinute();
    uint8_t getSecond();
    
    // Zeiteinstellung
    void setTime(const DateTime& dt);
    void setTime(uint16_t year, uint8_t month, uint8_t day,
                 uint8_t hour, uint8_t minute, uint8_t second);
    
    // Status
    bool isAvailable() const { return available; }
    bool hasLostPower();
    void updateHealth();
    bool hasBatteryWarning() const { return battery_warning; }
    bool hasOscillatorStopFlag() const { return oscillator_stop_flag; }
    bool hasTemperatureWarning() const { return temperature_warning; }
    bool hasHealthWarning() const { return battery_warning || temperature_warning || !available; }
    float getTemperatureC() const { return temperature_c; }
    
    // Synchronisation mit externen Quellen (z.B. NTP)
    void syncFromExternal(time_t unix_time);
    void syncFromExternal(const DateTime& dt);
    
private:
    uint8_t sda_pin;
    uint8_t scl_pin;
    bool available;
    bool power_was_lost;
    bool battery_warning = false;
    bool oscillator_stop_flag = false;
    bool temperature_warning = false;
    float temperature_c = NAN;
    unsigned long last_health_check_ms = 0;
};

#endif
