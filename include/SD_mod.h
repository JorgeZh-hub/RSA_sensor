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
#define DATA_BLOCK_SIZE 1000  // Tamaño del buffer (50 conjuntos * 20 bytes)

extern volatile bool sdWriteFlag;    // Bandera para indicar que hay datos para escribir
extern portMUX_TYPE sdMux; // Mutex para proteger la bandera

extern size_t bufferIndex;
extern size_t mybufferIndex;
extern byte writeBuffer[DATA_BLOCK_SIZE];
extern byte myBuffer[DATA_BLOCK_SIZE];

// Function prototypes
void writeToFile(const char *filename, const byte *buffer, size_t bufferSize);
void sd_task(void *pvParameters);

#endif