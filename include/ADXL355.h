#ifndef ADXL355_H
#define ADXL355_H

#include <Arduino.h>

// Direcciones de los registros de memoria del ADXL355
const int XDATA3 = 0x08;  // Byte más significativo del eje X
const int XDATA2 = 0x09;  // Byte medio del eje X
const int XDATA1 = 0x0A;  // Byte menos significativo del eje X
const int YDATA3 = 0x0B;  // Byte más significativo del eje Y
const int YDATA2 = 0x0C;  // Byte medio del eje Y
const int YDATA1 = 0x0D;  // Byte menos significativo del eje Y
const int ZDATA3 = 0x0E;  // Byte más significativo del eje Z
const int ZDATA2 = 0x0F;  // Byte medio del eje Z
const int ZDATA1 = 0x10;  // Byte menos significativo del eje Z
const int RANGE = 0x2C;   // Registro para configurar el rango de medición
const int POWER_CTL = 0x2D;  // Control de energía (modo de operación)
const int FIFO_DATA = 0x11;  // Datos del FIFO
const int STATUS_REG = 0x04;  // Registro de estado
const int FIFO_ENTRIES = 0x05;  // Número de entradas en el FIFO
const int FIFO_MAX_SIZE = 96;   // Tamaño máximo del FIFO en bytes

const int FILTER = 0x28;  // Configuración del filtro de datos
const int ODR_250_HZ = 0x04;  // Frecuencia de salida de datos a 250 Hz
const uint32_t TIME_INCREMENT = 4; // Incremento en milisegundos entre muestras
const int RESET = 0x2F;   // Registro de reinicio del dispositivo

// Valores del dispositivo
const int RANGE_2G = 0x01;       // Rango de medición de ±2G
const int STANDBY_MODE = 0x03;   // Configuración para modo de espera
const int MEASURE_MODE = 0x06;   // Configuración para modo de medición

// Operaciones de lectura y escritura
const int READ_BYTE = 0x01;      // Código para lectura de un byte
const int WRITE_BYTE = 0x00;     // Código para escritura de un byte

// Pines utilizados para la conexión con el ADXL355
#define ADXL_MISO 12  // Pin para la señal MISO (Master In Slave Out)
#define ADXL_MOSI 13  // Pin para la señal MOSI (Master Out Slave In)
#define ADXL_SCK 14   // Pin para la señal del reloj (SCK)
#define ADXL_CS 27    // Pin para la señal de selección de chip (CS)

extern SPIClass spiADXL; // SPI personalizado para el sensor ADXL355

// Prototipos de funciones
void writeRegister(byte thisRegister, byte thisValue);  // Escribe en un registro específico
unsigned int readRegistry(byte thisRegister);           // Lee un registro específico
void readFIFOData(uint8_t *buffer, int length);         // Lee datos del FIFO
float convert_data(uint8_t *data);                      // Convierte datos crudos a valores flotantes
int32_t raw_data(uint8_t *data);                        // Extrae datos crudos en formato de 32 bits
bool isDataReady();                                     // Verifica si hay datos listos en el sensor

#endif

