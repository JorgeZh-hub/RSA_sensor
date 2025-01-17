#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "ADXL355.h"
#include "RTC3231.h"
#include "SD_mod.h"
#include "credentials.h"
#include "GPS.h"

// Task handles
TaskHandle_t adx_taskHandle = NULL;  // Manejador de tarea para el acelerómetro
TaskHandle_t sync_taskHandle = NULL; // Manejador de tarea para la sincronización

volatile bool syncFlag = false;                  // Bandera que indica si se recibió la señal de sincronización
hw_timer_t *timer = NULL;                        // Puntero al temporizador utilizado para la generación de timestamps
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // Mutex para proteger la manipulación de la bandera de sincronización

// Configuración del temporizador para incrementar el timestamp
void setupTimer()
{
    timer = timerBegin(0, 80, true); // Timer 0, prescaler 80 (1 MHz = 1 µs)
    timerAttachInterrupt(timer, []()
                         {
                             millis_timestamp++; // Incrementa el timestamp en cada interrupción del temporizador
                         },
                         true);
    timerAlarmWrite(timer, 1000, true); // 1000 µs = 1 ms
    timerAlarmEnable(timer);            // Habilita la alarma del temporizador
}

// Función de interrupción cuando se recibe un pulso de 1 Hz (SQW) del RTC
void IRAM_ATTR sqwInterrupt()
{
    portENTER_CRITICAL(&mux); // Inicia sección crítica para proteger la bandera de sincronización
    syncFlag = true;          // Marca que el pulso ha sido recibido
    portEXIT_CRITICAL(&mux);  // Finaliza sección crítica
}

// Tarea de sincronización que monitorea la llegada del pulso
void sync_task(void *pvParameters)
{
    while (true)
    {
        if (syncFlag)
        {
            portENTER_CRITICAL(&mux); // Inicia sección crítica
            syncFlag = false;         // Resetea la bandera
            portEXIT_CRITICAL(&mux);  // Finaliza sección crítica
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Retardo de 1 ms para evitar uso excesivo de CPU
    }
}

// Función para generar los timestamps en el buffer
void generate_buffer_timestamp(uint32_t *buffer_timestamp, int num_samples, uint32_t actual_timestamp)
{
    for (int i = num_samples - 1; i >= 0; i--)
    {
        buffer_timestamp[i] = actual_timestamp - (num_samples - 1 - i) * TIME_INCREMENT; // Rellena los timestamps con un incremento
    }
}

// Tarea que lee los datos del acelerómetro y los escribe en la SD
void acelerometroTask(void *pvParameters)
{
    while (true)
    {
        if (isDataReady())
        {                                                 // Verifica si hay datos listos para ser leídos
            int fifoEntries = readRegistry(FIFO_ENTRIES); // Lee el número de entradas en el FIFO
            int samplesToRead = fifoEntries * 3;          // Calcula cuántos bytes de datos leer (3 bytes por eje)

            if (samplesToRead > 0 && samplesToRead % 9 == 0)
            {                                                                                          // Asegura que el número de bytes sea múltiplo de 9 (3 ejes x 3 muestras)
                uint8_t *buffer = (uint8_t *)malloc(samplesToRead * sizeof(uint8_t));                  // Asigna memoria para los datos
                uint32_t *buffer_timestamp = (uint32_t *)malloc(samplesToRead / 9 * sizeof(uint32_t)); // Asigna memoria para los timestamps

                generate_buffer_timestamp(buffer_timestamp, samplesToRead / 9, millis_timestamp); // Genera los timestamps basados en el tiempo actual

                readFIFOData(buffer, samplesToRead); // Lee los datos del FIFO

                // Procesa los datos en bloques de 9 bytes (3 ejes x 3 muestras)
                for (int i = 0; i < samplesToRead; i += 9)
                {
                    int32_t x_raw = raw_data(&buffer[i]);     // Obtiene los datos crudos del eje X
                    int32_t y_raw = raw_data(&buffer[i + 3]); // Obtiene los datos crudos del eje Y
                    int32_t z_raw = raw_data(&buffer[i + 6]); // Obtiene los datos crudos del eje Z

                    uint8_t dataToWrite[20]; // Conjunto de datos de 20 bytes (fecha + timestamp + 3 ejes)

                    if (x_raw == 0 && y_raw == 0 && z_raw == 0)
                    { // Si los valores de los ejes son 0, no se procesan
                        continue;
                    }
#ifdef DEBUG
                    // Imprime los datos de cada muestra para depuración
                    Serial.printf("%d,  %d,     %d,    %d\n", buffer_timestamp[i / 9], x_raw, y_raw, z_raw);
#endif

                    // Copia la fecha y los datos al buffer final
                    memcpy(dataToWrite, (const void *)&fecha, sizeof(fecha));
                    memcpy(dataToWrite + sizeof(fecha), (const void *)&buffer_timestamp[i / 9], sizeof(buffer_timestamp[i / 9]));
                    memcpy(dataToWrite + sizeof(fecha) + sizeof(buffer_timestamp[i / 9]), (const void *)&x_raw, sizeof(x_raw));
                    memcpy(dataToWrite + sizeof(fecha) + sizeof(buffer_timestamp[i / 9]) + sizeof(x_raw), (const void *)&y_raw, sizeof(y_raw));
                    memcpy(dataToWrite + sizeof(fecha) + sizeof(buffer_timestamp[i / 9]) + sizeof(x_raw) + sizeof(y_raw), (const void *)&z_raw, sizeof(z_raw));

                    // Verifica si hay espacio en el buffer y escribe en la tarjeta SD
                    if (bufferIndex + sizeof(dataToWrite) <= DATA_BLOCK_SIZE)
                    {
                        memcpy(&writeBuffer[bufferIndex], dataToWrite, sizeof(dataToWrite)); // Copia los datos al buffer de escritura
                        bufferIndex += sizeof(dataToWrite);                                  // Actualiza el índice del buffer
                    }
                    else
                    {
                        // Si el buffer está lleno, escribe los datos en el archivo
                        writeToFile("/data0.bin", writeBuffer, bufferIndex);
                        bufferIndex = 0; // Resetea el índice del buffer
                    }
                }

                free(buffer);           // Libera la memoria utilizada para los datos
                free(buffer_timestamp); // Libera la memoria utilizada para los timestamps
            }
        }
    }
}

void setup()
{
#ifdef DEBUG
    Serial.begin(SERIAL_SPEED); // Inicializa la comunicación serie para depuración
#endif

    Wire.begin(I2C_SDA, I2C_SCL); // Inicializa la comunicación I2C para dispositivos como el RTC

    ////////////////////////////////////  Inicialización del módulo RTC ////////////////////////////////////////

    if (!rtc.begin())
    { // Verifica si el RTC está disponible
#ifdef DEBUG
        Serial.println("Couldn't find RTC"); // Imprime error si no se encuentra el RTC
#endif
        while (1)
            ; // Detiene la ejecución si el RTC no está disponible
    }

    GPS_init(); // Inicializa el GPS

    // Verifica si el RTC ha perdido energía (por ejemplo, si la batería está descargada)
    if (rtc.lostPower())
    {
        updateRTCfromGPS(); // Actualiza el RTC utilizando el GPS
    }
    else
    {
#ifdef DEBUG
        Serial.println("RTC is running"); // Imprime que el RTC está funcionando
#endif

        // Obtiene la hora actual del RTC
        DateTime now = rtc.now();

        // Convierte la hora y fecha a formato epoch (segundos desde 1970)
        uint32_t hora = now.hour() * 3600 + now.minute() * 60 + now.second();
        uint32_t fecha = now.year() * 10000 + now.month() * 100 + now.day();

        // Actualiza los contadores globales
        millis_timestamp = hora * 1000; // Timestamp en milisegundos
        ::fecha = fecha;                // Fecha en formato YYYYMMDD

// Imprime los valores para depuración
#ifdef DEBUG
        Serial.printf("Hora RTC: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
        Serial.printf("Fecha RTC: %04d-%02d-%02d\n", now.year(), now.month(), now.day());
#endif
    }

    // Configura la salida de onda cuadrada (SQW) a 1 Hz
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);

    pinMode(SQW_INT, INPUT_PULLUP);                                         // Configura el pin de interrupción de SQW con resistencia pull-up
    attachInterrupt(digitalPinToInterrupt(SQW_INT), sqwInterrupt, FALLING); // Configura la interrupción en el flanco de bajada

    setupTimer(); // Configura el temporizador para la actualización del timestamp

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////// Inicialización VSPI (SD) /////////////////////////////////////////////////
// Configura el bus SPI para la comunicación con la tarjeta SD
#ifdef DEBUG
    Serial.println("Iniciando SD..."); // Imprime mensaje de inicio para la SD
#endif
    spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); // Inicializa el bus SPI para la SD

    // Inicializa la tarjeta SD
    if (!SD.begin(SD_CS, spiSD, SD_FREQ))
    { // Verifica si la tarjeta SD se inicializa correctamente
#ifdef DEBUG
        Serial.println("Error al inicializar la tarjeta SD"); // Imprime error si la SD no se inicializa
#endif
    }
    else
    {
#ifdef DEBUG
        Serial.println("Tarjeta SD inicializada correctamente"); // Imprime éxito si la SD se inicializa correctamente
#endif
    }

/////////////////////////////////////////// Inicio y configuración del ADXL355 ///////////////////////////////////////
#ifdef DEBUG
    Serial.println("Iniciando ADXL355..."); // Imprime mensaje de inicio para el acelerómetro ADXL355
#endif

    spiADXL.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS); // Inicializa el bus SPI para el ADXL355
    pinMode(ADXL_CS, OUTPUT);                               // Configura el pin CS del ADXL355 como salida

    // Reinicia el ADXL355 antes de configurarlo
    writeRegister(RESET, 0x52); // Comando para reiniciar el ADXL355
    delay(100);                 // Espera 100 ms para que el reinicio se complete

    // Configura el ADXL355
    writeRegister(POWER_CTL, STANDBY_MODE); // Pone el ADXL355 en modo de espera
    writeRegister(RANGE, RANGE_2G);         // Configura el rango del acelerómetro a 2G
    writeRegister(FILTER, ODR_250_HZ);      // Configura la frecuencia de salida del filtro a 250 Hz
    writeRegister(POWER_CTL, MEASURE_MODE); // Pone el ADXL355 en modo de medición
    delay(100);                             // Espera 100 ms para que la configuración se complete
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Crea las tareas para el acelerómetro y la sincronización
    xTaskCreatePinnedToCore(acelerometroTask, "AcelerometroTask", 4096, NULL, 1, &adx_taskHandle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(sync_task, "SYNCTask", 2048, NULL, 1, &sync_taskHandle, tskNO_AFFINITY);
}

void loop() {} // La función loop está vacía, ya que el código se maneja en las tareas
