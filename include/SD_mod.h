#ifndef RTC3231_H

#include <Arduino.h>
#include <SD.h>

// Configuración de pines para el módulo SD (VSPI)
#define SD_MISO 19
#define SD_MOSI 23
#define SD_SCK 18
#define SD_CS 5
extern SPIClass spiSD;

const int SD_FREQ = 4000000;

// Function prototypes
void writeToFile(const char *filename, const char *message);
void sd_task(void *pvParameters);

#endif