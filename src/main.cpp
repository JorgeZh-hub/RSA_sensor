#include <Arduino.h>
#include <SPI.h>
#include "ADXL355.h"

/*
CONEXION ADXL355

    ESP32     ADXL
    3V3        P1_2
    GND        P1_3
    D2         P2_4
    D18        P2_5
    D19        P2_3
    D23        P2_6
*/

// Declaración de la tarea para el acelerómetro
void acelerometroTask(void *pvParameters);

void setup() {
    Serial.begin(115200);
    SPI.begin();
    pinMode(CHIP_SELECT_PIN_ADXL355, OUTPUT);

    // Configuración del ADXL355
    writeRegister(POWER_CTL, STANDBY_MODE);
    writeRegister(RANGE, RANGE_2G);
    writeRegister(FILTER, ODR_250_HZ);
    writeRegister(POWER_CTL, MEASURE_MODE);
    delay(100);

    // Crear la tarea para leer datos del acelerómetro en el núcleo 1
    xTaskCreatePinnedToCore(
        acelerometroTask,      // Función de la tarea
        "Acelerometro Task",   // Nombre de la tarea
        8192,                  // Tamaño de la pila en bytes
        NULL,                  // Parámetros de la tarea
        1,                     // Prioridad de la tarea
        NULL,                  // Handle de la tarea (opcional)
        tskNO_AFFINITY         // Asignar la tarea al núcleo 
    );
}

void loop() {}

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

