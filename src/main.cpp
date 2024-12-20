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
TaskHandle_t sync_taskHandle = NULL;


volatile bool syncFlag = false;
hw_timer_t *timer = NULL;
volatile uint32_t millisCounter = 0;

void IRAM_ATTR sqwInterrupt() {
    syncFlag = true; // Marca la llegada del pulso
}

void setupTimer() {
    timer = timerBegin(0, 80, true); // Timer 0, prescaler 80 (1 MHz = 1 µs)
    timerAttachInterrupt(timer, []() {
        millisCounter++;
        millisCounter = millisCounter % 1000;
    }, true);
    timerAlarmWrite(timer, 1000, true); // 1000 µs = 1 ms
    timerAlarmEnable(timer);
}

void sync_task(void *pvParameters) {
    while (true) {
        if (syncFlag) {
            syncFlag = false;
            millisCounter = 0; // Resetea el contador al detectar el pulso de SQW
        }
        vTaskDelay(pdMS_TO_TICKS(3)); // Pausa de 100 ms para evitar sobrecargar el puerto serie
    }
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

                    Serial.printf("%02d, %.3f, %.3f, %.3f\n",millisCounter, x, y, z);
                    /*DateTime now = rtc.now();
                    char message[100];
                    sprintf(message, "%04d/%02d/%02d %02d:%02d:%02d:%02d, %.2f, %.2f, %.2f", 
                            now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), millisCounter,
                            x, y, z);
                    writeToFile("/data.txt", message);*/
                }

                free(buffer);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

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

    rtc.writeSqwPinMode(DS3231_SquareWave1Hz); // Configura SQW a 1 Hz

    pinMode(34, INPUT_PULLUP); 
    attachInterrupt(digitalPinToInterrupt(34), sqwInterrupt, FALLING); // Interrumpe en el flanco de bajada

    setupTimer();
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
    //xTaskCreatePinnedToCore(rtc_task, "RTCTask", 4096, NULL, 1, &rtc_taskHandle, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(sd_task, "SDTask", 4096, NULL, 1, &sd_taskHandle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(sync_task, "SYNCTask", 2048, NULL, 1, &sync_taskHandle, tskNO_AFFINITY);


}

void loop() {}