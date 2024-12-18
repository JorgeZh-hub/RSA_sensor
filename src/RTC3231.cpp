#include "RTC3231.h"
RTC_DS3231 rtc;
void updateRTCFromNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(1000);
        now = time(nullptr);
    }

    time(&now);
    struct tm *local_tm = localtime(&now);
    if (local_tm != NULL) {
        rtc.adjust(DateTime(local_tm->tm_year + 1900, local_tm->tm_mon + 1, local_tm->tm_mday, 
                            local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec));
        Serial.println("RTC updated from NTP");
    } else {
        Serial.println("Failed to get time");
    }
}


void rtc_task(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = 1000 / portTICK_PERIOD_MS;

    while (true) {
        DateTime now = rtc.now();
        Serial.printf("Date: %04d/%02d/%02d \t", now.year(), now.month(), now.day());
        Serial.printf("Time: %02d:%02d:%02d \n", now.hour(), now.minute(), now.second());
        
        vTaskDelayUntil(&lastWakeTime, interval);
    }
}