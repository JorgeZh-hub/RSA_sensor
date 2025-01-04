#include "SD_mod.h"

SPIClass spiSD(VSPI);
volatile bool sdWriteFlag = false;    // Bandera para indicar que hay datos para escribir
portMUX_TYPE sdMux = portMUX_INITIALIZER_UNLOCKED; // Mutex para proteger la bandera
size_t bufferIndex = 0; // Índice del buffer de escritura
size_t mybufferIndex = 0;
byte writeBuffer[DATA_BLOCK_SIZE]; // Buffer de escritura
byte myBuffer[DATA_BLOCK_SIZE];

void writeToFile(const char *filename, const byte *buffer, size_t bufferSize) {
    File file = SD.open(filename, FILE_APPEND);
    if (file) {
        file.write(buffer, bufferSize);  
        file.close();
    } else {
        #ifdef DEBUG
        Serial.println("SD Write Failed");
        #endif
    }
}