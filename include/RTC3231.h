#ifndef RTC3231_H

#define DEBUG

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <time.h>

extern RTC_DS3231 rtc;

constexpr const char* ntpServer = "pool.ntp.org";
constexpr long gmtOffset_sec = -5 * 3600;
constexpr int daylightOffset_sec = 0;

extern volatile uint32_t millis_timestamp; // Declaración de la variable global
extern uint32_t hora;                   // Declaración de la hora
extern uint32_t fecha;                  // Declaración de la fecha

// Function prototypes
void updateRTCFromNTP();
void updateRTCfromGPS();
void rtc_task(void *pvParameters);
uint32_t getRTCTimeMillis();

#endif