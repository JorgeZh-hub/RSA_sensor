// lib/ADXL355/ADXL355.cpp
#include <SPI.h>
#include "ADXL355.h"
#include "esp_task_wdt.h"
//#include <freertos/semphr.h>

//SemaphoreHandle_t xSpiMutex;

SPIClass spiADXL(HSPI);


void writeRegister(byte thisRegister, byte thisValue) {
    //xSemaphoreTake(xSpiMutex, portMAX_DELAY);
    byte dataToSend = (thisRegister << 1) | WRITE_BYTE;
    digitalWrite(ADXL_CS, LOW);
    spiADXL.transfer(dataToSend);
    spiADXL.transfer(thisValue);
    digitalWrite(ADXL_CS, HIGH);
    //xSemaphoreGive(xSpiMutex);
}

unsigned int readRegistry(byte thisRegister) {
    //xSemaphoreTake(xSpiMutex, portMAX_DELAY);
    unsigned int result = 0;
    byte dataToSend = (thisRegister << 1) | READ_BYTE;
    digitalWrite(ADXL_CS, LOW);
    spiADXL.transfer(dataToSend);
    result = spiADXL.transfer(0x00);
    digitalWrite(ADXL_CS, HIGH);
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
    digitalWrite(ADXL_CS, LOW);
    byte dataToSend = (FIFO_DATA << 1) | READ_BYTE;
    spiADXL.transfer(dataToSend);
    for (int i = 0; i < length; i++) {
        buffer[i] = spiADXL.transfer(0x00);
    }
    digitalWrite(ADXL_CS, HIGH);
    //xSemaphoreGive(xSpiMutex);
}


// Helper function: Check if data is ready
bool isDataReady() {
  uint8_t status = readRegistry(STATUS_REG);
  return (status & 0x01); // DTA_RDY bit
}

void acelerometroTask(void *pvParameters) {
    while (true) {
        if (isDataReady()) {
            int fifoEntries = readRegistry(FIFO_ENTRIES);
            int samplesToRead = fifoEntries * 3; // 3 bytes per axis (X, Y, Z)

            if (samplesToRead > 0) {
                uint8_t *buffer = (uint8_t *)malloc(samplesToRead * sizeof(uint8_t));
                if (buffer == NULL) {
                    Serial.println("Memory allocation failed!");
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    continue;
                }

                readFIFOData(buffer, samplesToRead);

                for (int i = 0; i < samplesToRead; i += 9) { // Each set is 9 bytes
                    float x = convert_data(&buffer[i]);
                    float y = convert_data(&buffer[i + 3]);
                    float z = convert_data(&buffer[i + 6]);

                    Serial.printf("%.3f, %.3f, %.3f\n", x, y, z);
                }

                free(buffer);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}