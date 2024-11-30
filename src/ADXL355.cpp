// lib/ADXL355/ADXL355.cpp
#include <SPI.h>
#include "ADXL355.h"
//#include <freertos/semphr.h>

//SemaphoreHandle_t xSpiMutex;


void writeRegister(byte thisRegister, byte thisValue) {
    //xSemaphoreTake(xSpiMutex, portMAX_DELAY);
    byte dataToSend = (thisRegister << 1) | WRITE_BYTE;
    digitalWrite(CHIP_SELECT_PIN_ADXL355, LOW);
    SPI.transfer(dataToSend);
    SPI.transfer(thisValue);
    digitalWrite(CHIP_SELECT_PIN_ADXL355, HIGH);
    //xSemaphoreGive(xSpiMutex);
}

unsigned int readRegistry(byte thisRegister) {
    //xSemaphoreTake(xSpiMutex, portMAX_DELAY);
    unsigned int result = 0;
    byte dataToSend = (thisRegister << 1) | READ_BYTE;
    digitalWrite(CHIP_SELECT_PIN_ADXL355, LOW);
    SPI.transfer(dataToSend);
    result = SPI.transfer(0x00);
    digitalWrite(CHIP_SELECT_PIN_ADXL355, HIGH);
    //xSemaphoreGive(xSpiMutex);
    return result;
}

float convert_data(uint8_t *data){
  int32_t raw = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (int32_t)data[2]>>4;   // Reubicación de bits
  if (raw & 0x80000) raw -= 0x100000;     // Módulo 2
  float acceleration = raw * 0.007476;    // Conversion a cm/s^2
  return acceleration;
}

void readFIFOData(uint8_t *buffer, int length) {
    //xSemaphoreTake(xSpiMutex, portMAX_DELAY);
    digitalWrite(CHIP_SELECT_PIN_ADXL355, LOW);
    byte dataToSend = (FIFO_DATA << 1) | READ_BYTE;
    SPI.transfer(dataToSend);
    for (int i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(CHIP_SELECT_PIN_ADXL355, HIGH);
    //xSemaphoreGive(xSpiMutex);
}


// Helper function: Check if data is ready
bool isDataReady() {
  uint8_t status = readRegistry(STATUS_REG);
  return (status & 0x01); // DTA_RDY bit
}