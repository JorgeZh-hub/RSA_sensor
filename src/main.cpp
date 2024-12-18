#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "ADXL355.h"
#include "RTC3231.h"
#include "SD_mod.h"
#include "credentials.h"


// Task handles
TaskHandle_t rtc_taskHandle = NULL;
TaskHandle_t sd_taskHandle = NULL;
TaskHandle_t adx_taskHandle = NULL;



void setup() {
    Serial.begin(115200);

    Wire.begin(21, 22);   // Comunicación I2C

    ////////////////////////////////////  Inicialización del módulo RTC ////////////////////////////////////////

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (rtc.lostPower()) {
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            
            Serial.println("Connecting WiFi...");
        }
        Serial.println("WiFi Connected");

        updateRTCFromNTP();
    } else {
        Serial.println("RTC is running");
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////// Inicialización VSPI (SD) /////////////////////////////////////////////////
    // Configurar el bus VSPI para el módulo SD
    spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    Serial.println("Iniciando SD...");

    // Inicializar SD
    if (!SD.begin(SD_CS, spiSD, SD_FREQ)) {
        Serial.println("Error al inicializar la tarjeta SD");
    } else {
        Serial.println("Tarjeta SD inicializada correctamente");
    }

    /////////////////////////////////////////// Inicio y configuración del ADXL355 ///////////////////////////////////////
    
    spiADXL.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS);
    pinMode(ADXL_CS, OUTPUT);

    // Configuración del ADXL355
    writeRegister(POWER_CTL, STANDBY_MODE);
    writeRegister(RANGE, RANGE_2G);
    writeRegister(FILTER, ODR_250_HZ);
    writeRegister(POWER_CTL, MEASURE_MODE);
    delay(100);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Crear tareas
    xTaskCreatePinnedToCore(acelerometroTask, "AcelerometroTask", 8192, NULL, 1,  &adx_taskHandle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(rtc_task, "RTCTask", 4096, NULL, 1, &rtc_taskHandle, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(sd_task, "SDTask", 4096, NULL, 1, &sd_taskHandle, tskNO_AFFINITY);

}

void loop() {}