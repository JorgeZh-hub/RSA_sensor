#ifndef SD_H
#define SD_H

#include <Arduino.h>
#include <SD.h>

// Configuración de pines para la comunicación SPI con el módulo SD (VSPI)
#define SD_MISO 19     // Pin de entrada de datos (Master In Slave Out)
#define SD_MOSI 23     // Pin de salida de datos (Master Out Slave In)
#define SD_SCK 18      // Pin de reloj (Serial Clock)
#define SD_CS 5        // Pin de selección de chip (Chip Select)
extern SPIClass spiSD; // Instancia de la clase SPI para la comunicación con el módulo SD

const int SD_FREQ = 4000000; // Frecuencia de la comunicación SPI con la tarjeta SD (4 MHz)
#define DATA_BLOCK_SIZE 1000 // Tamaño del buffer utilizado para almacenar datos a escribir (1000 bytes)

// Variables externas para gestionar la escritura de datos en el archivo
extern volatile bool sdWriteFlag; // Bandera para indicar que hay datos listos para escribir en el archivo
extern portMUX_TYPE sdMux;        // Mutex para proteger la bandera y evitar problemas de concurrencia

// Variables para gestionar los buffers de datos a escribir en la tarjeta SD
extern size_t bufferIndex;                // Índice de la posición actual del buffer principal
extern byte writeBuffer[DATA_BLOCK_SIZE]; // Buffer para almacenar los datos a escribir

// Prototipos de las funciones
void writeToFile(const char *filename, const byte *buffer, size_t bufferSize); // Función para escribir en un archivo de la tarjeta SD

#endif // SD_H
