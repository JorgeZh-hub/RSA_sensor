#ifndef ADXL355_H
#define ADXL355_H

#include <Arduino.h>

/*
CONEXION ADXL355

    ESP32     ADXL
    3V3        P1_2
    GND        P1_3
    D2->15         P2_4
    D18->14        P2_5
    D19->12        P2_3
    D23->13        P2_6
*/


// ADXL355 memory register addresses
const int XDATA3 = 0x08;
const int XDATA2 = 0x09;
const int XDATA1 = 0x0A;
const int YDATA3 = 0x0B;
const int YDATA2 = 0x0C;
const int YDATA1 = 0x0D;
const int ZDATA3 = 0x0E;
const int ZDATA2 = 0x0F;
const int ZDATA1 = 0x10;
const int RANGE = 0x2C;
const int POWER_CTL = 0x2D;
const int FIFO_DATA = 0x11;
const int STATUS_REG = 0x04;
const int FIFO_ENTRIES = 0x05;
const int FIFO_MAX_SIZE = 96;

const int FILTER = 0x28;
const int ODR_250_HZ = 0x04;  
const int RESET = 0x2F;  

// Device values
const int RANGE_2G = 0x01;
const int STANDBY_MODE = 0x03;    // DTRDY, NO TEMP, STANDBY
const int MEASURE_MODE = 0x06;    // NO DTRDY, NO TEMP, MEASURE

// Operations
const int READ_BYTE = 0x01;
const int WRITE_BYTE = 0x00;

// Pins used for the connection with ADXL355Z

#define ADXL_MISO 12
#define ADXL_MOSI 13
#define ADXL_SCK 14
#define ADXL_CS 15

extern SPIClass spiADXL;

//const int CHIP_SELECT_PIN_SD = 5;

// Function prototypes
void writeRegister(byte thisRegister, byte thisValue);
unsigned int readRegistry(byte thisRegister);
void readFIFOData(uint8_t *buffer, int length);
float convert_data(uint8_t *data);
int32_t raw_data(uint8_t *data);
bool isDataReady();
//void acelerometroTask(void *pvParameters);

#endif
