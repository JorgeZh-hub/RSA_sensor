#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "ADXL355.h"
#include "RTC3231.h"
#include "SD_mod.h"
#include "credentials.h"
#include "GPS.h"


// Task handles
TaskHandle_t adx_taskHandle = NULL;
TaskHandle_t sync_taskHandle = NULL; 


volatile bool syncFlag = false;
hw_timer_t *timer = NULL;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // Inicializa el mutex



void setupTimer() {
    timer = timerBegin(0, 80, true); // Timer 0, prescaler 80 (1 MHz = 1 µs)
    timerAttachInterrupt(timer, []() {
        millis_timestamp++;
    }, true);
    timerAlarmWrite(timer, 1000, true); // 1000 µs = 1 ms
    timerAlarmEnable(timer);
}

void IRAM_ATTR sqwInterrupt() {
    portENTER_CRITICAL(&mux);  // Inicia sección crítica con el mutex
    syncFlag = true;  // Marca la llegada del pulso
    portEXIT_CRITICAL(&mux);   // Finaliza sección crítica con el mutex
}

void sync_task(void *pvParameters) {
    while (true) {
        if (syncFlag) {
            portENTER_CRITICAL(&mux);  // Inicia sección crítica con el mutex
            syncFlag = false;          // Resetea la bandera
            portEXIT_CRITICAL(&mux);   // Finaliza sección crítica con el mutex
        }
        vTaskDelay(pdMS_TO_TICKS(1));  
    }
}

void generate_buffer_timestamp(uint32_t *buffer_timestamp, int num_samples, uint32_t actual_timestamp) {
    const uint32_t TIME_INCREMENT = 4; // Incremento en milisegundos

    for (int i = num_samples - 1; i >= 0; i--) {
        buffer_timestamp[i] = actual_timestamp - (num_samples - 1 - i) * TIME_INCREMENT;
    }
}



void acelerometroTask(void *pvParameters) {
    while (true) {
        if (isDataReady()) {
            int fifoEntries = readRegistry(FIFO_ENTRIES);
            int samplesToRead = fifoEntries * 3; // 3 bytes por eje (X, Y, Z)

            if (samplesToRead > 0 && samplesToRead % 9 == 0) {
                //uint8_t *buffer = (uint8_t *)malloc(samplesToRead * sizeof(uint8_t));
                uint8_t *buffer = (uint8_t *)malloc(samplesToRead*sizeof(uint8_t));
                uint32_t *buffer_timestamp = (uint32_t *)malloc(samplesToRead/9*sizeof(uint32_t));
                
                generate_buffer_timestamp(buffer_timestamp, samplesToRead/9, millis_timestamp);

                readFIFOData(buffer, samplesToRead);

                for (int i = 0; i < samplesToRead; i += 9) {
                    int32_t x_raw = raw_data(&buffer[i]);
                    int32_t y_raw = raw_data(&buffer[i + 3]);
                    int32_t z_raw = raw_data(&buffer[i + 6]);

                    uint8_t dataToWrite[20];  // Tamaño del conjunto: 20 bytes

                    if (x_raw == 0 && y_raw == 0 && z_raw == 0) {
                        continue;
                    }
                    #ifdef DEBUG
                        Serial.printf("%d,  %d,     %d,    %d\n", buffer_timestamp[i/9], x_raw, y_raw, z_raw);
                    #endif
                    
                    memcpy(dataToWrite, (const void *)&fecha, sizeof(fecha));
                    memcpy(dataToWrite + sizeof(fecha), (const void *)&buffer_timestamp[i/9], sizeof(buffer_timestamp[i/9]));
                    memcpy(dataToWrite + sizeof(fecha) + sizeof(buffer_timestamp[i/9]), (const void *)&x_raw, sizeof(x_raw));
                    memcpy(dataToWrite + sizeof(fecha) + sizeof(buffer_timestamp[i/9]) + sizeof(x_raw), (const void *)&y_raw, sizeof(y_raw));
                    memcpy(dataToWrite + sizeof(fecha) + sizeof(buffer_timestamp[i/9]) + sizeof(x_raw) + sizeof(y_raw), (const void *)&z_raw, sizeof(z_raw));
                    
                    
                    
                    if (bufferIndex + sizeof(dataToWrite) <= DATA_BLOCK_SIZE) {
                        memcpy(&writeBuffer[bufferIndex], dataToWrite, sizeof(dataToWrite));
                        bufferIndex += sizeof(dataToWrite);
                    } else {
                        writeToFile("/data0.bin", writeBuffer, bufferIndex);
                        bufferIndex = 0;
                    }
                }

                free(buffer);
                free(buffer_timestamp);
            }
        }
    }
}



void setup() {
    #ifdef DEBUG
        Serial.begin(115200);
    #endif

    Wire.begin(21, 22);   // Comunicación I2C

    ////////////////////////////////////  Inicialización del módulo RTC ////////////////////////////////////////

    if (!rtc.begin()) {
        #ifdef DEBUG
            Serial.println("Couldn't find RTC");
        #endif
        while (1);
    }

    GPS_init();           // Inicializa el GPS
    
    if (rtc.lostPower()) {
        updateRTCfromGPS(); // Actualiza desde el GPS si el RTC perdió energía
    } else {
        #ifdef DEBUG
            Serial.println("RTC is running"); 
        #endif
        
        // Obtén la hora actual desde el RTC
        DateTime now = rtc.now();

        // Convertir los valores a uint32_t epoch
        uint32_t hora = now.hour() * 3600 + now.minute() * 60 + now.second();
        uint32_t fecha = now.year() * 10000 + now.month() * 100 + now.day();

        // Actualiza los contadores globales
        millis_timestamp = hora*1000;
        ::fecha = fecha; 

        // Imprime los valores para depuración
        #ifdef DEBUG
        Serial.printf("Hora RTC: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
        Serial.printf("Fecha RTC: %04d-%02d-%02d\n", now.year(), now.month(), now.day());
        #endif
    }

    

    rtc.writeSqwPinMode(DS3231_SquareWave1Hz); // Configura SQW a 1 Hz

    pinMode(34, INPUT_PULLUP); 
    attachInterrupt(digitalPinToInterrupt(34), sqwInterrupt, FALLING); // Interrumpe en el flanco de bajada

    setupTimer();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////// Inicialización VSPI (SD) /////////////////////////////////////////////////
    // Configurar el bus VSPI para el módulo SD
    #ifdef DEBUG
        Serial.println("Iniciando SD...");
    #endif
    spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);


    // Inicializar SD
    if (!SD.begin(SD_CS, spiSD, SD_FREQ)) {
        #ifdef DEBUG
            Serial.println("Error al inicializar la tarjeta SD");
        #endif
    } else {
        #ifdef DEBUG
            Serial.println("Tarjeta SD inicializada correctamente");
        #endif
    }


    /////////////////////////////////////////// Inicio y configuración del ADXL355 ///////////////////////////////////////
    #ifdef DEBUG
        Serial.println("Iniciando ADXL355...");
    #endif
    
    spiADXL.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS);
    pinMode(ADXL_CS, OUTPUT);

    // Reiniciar el ADXL355 antes de configurarlo
    writeRegister(RESET, 0x52);  // Comando de reinicio del ADXL355
    delay(100);                  // Esperar un tiempo para que complete el reinicio
    // Configuración del ADXL355
    writeRegister(POWER_CTL, STANDBY_MODE);
    writeRegister(RANGE, RANGE_2G);
    writeRegister(FILTER, ODR_250_HZ);
    writeRegister(POWER_CTL, MEASURE_MODE);
    delay(100);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Crear tareas
    xTaskCreatePinnedToCore(acelerometroTask, "AcelerometroTask", 4096, NULL, 1,  &adx_taskHandle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(sync_task, "SYNCTask", 2048, NULL, 1, &sync_taskHandle, tskNO_AFFINITY);


}

void loop() {}