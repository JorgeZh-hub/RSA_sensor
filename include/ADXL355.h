#ifndef ADXL355_H
#define ADXL355_H

#include <Arduino.h>


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

const int FILTER = 0x28;
const int ODR_250_HZ = 0x04;    

// Device values
const int RANGE_2G = 0x01;
const int STANDBY_MODE = 0x03;    // DTRDY, NO TEMP, STANDBY
const int MEASURE_MODE = 0x06;    // NO DTRDY, NO TEMP, MEASURE

// Operations
const int READ_BYTE = 0x01;
const int WRITE_BYTE = 0x00;

// Pins used for the connection with ADXL355Z
const int CHIP_SELECT_PIN_ADXL355 = 2;
//const int CHIP_SELECT_PIN_SD = 5;

// Function prototypes
void writeRegister(byte thisRegister, byte thisValue);
unsigned int readRegistry(byte thisRegister);
void readFIFOData(uint8_t *buffer, int length);
float convert_data(uint8_t *data);
bool isDataReady();

#endif