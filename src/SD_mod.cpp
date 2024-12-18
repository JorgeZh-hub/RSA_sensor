#include "SD_mod.h"
#include "RTC3231.h"

SPIClass spiSD(VSPI);

void writeToFile(const char *filename, const char *message) {
    File file = SD.open(filename, FILE_APPEND);
    if (file) {
        file.println(message);
        file.close();
        Serial.println("SD Write Success");
    } else {
        Serial.println("SD Write Failed");
    }
}

void sd_task(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true) {
        DateTime now = rtc.now();
        char message[100];
        sprintf(message, "%04d/%02d/%02d %02d:%02d:%02d, %.2f, %.2f, %.2f, %.2f, %.2f", 
                now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), 
                3.0, 3.0, 3.0, 3.0, 3.0);
        writeToFile("/data.txt", message);
        vTaskDelayUntil(&lastWakeTime, 10000 / portTICK_PERIOD_MS);
        Serial.printf("                    ");
    }
}

