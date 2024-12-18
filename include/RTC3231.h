#ifndef RTC3231_H

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <time.h>

extern RTC_DS3231 rtc;

// Servidor NTP
constexpr const char* ntpServer = "pool.ntp.org";
constexpr long gmtOffset_sec = -5 * 3600;
constexpr int daylightOffset_sec = 0;

// Function prototypes
void updateRTCFromNTP();
void rtc_task(void *pvParameters);

#endif