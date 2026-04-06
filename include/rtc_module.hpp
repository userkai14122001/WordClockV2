#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

#ifndef RTC_SDA_PIN
#define RTC_SDA_PIN GPIO_NUM_6
#endif

#ifndef RTC_SCL_PIN
#define RTC_SCL_PIN GPIO_NUM_7
#endif

bool rtc_init(uint8_t sda_pin = RTC_SDA_PIN, uint8_t scl_pin = RTC_SCL_PIN);
void rtc_set_time_utc(const DateTime& dt_utc);
DateTime rtc_get_time_local();
bool rtc_is_available();
bool rtc_read_oscillator_stop_flag();
float rtc_get_temperature_c();