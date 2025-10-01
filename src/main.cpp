#include <Arduino.h>
#include <SPI.h>
#include "ADXL355.h"
#include "RTC3231.h"
#include "SD_mod.h"
#include "WiFi_mod.h"
#include "GPS.h"
#include "Mqtt_mod.h"
#include "credentials.h"

uint32_t fecha_actual = 0;           // Fecha leída del RTC (YYYYMMDD)
uint32_t fecha_anterior = 0;         // Fecha del último archivo
char fileName[20] = "/00000000.bin"; // Nombre actual del archivo

// Task handles
TaskHandle_t adx_taskHandle = NULL;  // Manejador de tarea para el acelerómetro
TaskHandle_t wifi_taskHandle = NULL; // Manejador de tarea para la sincronización

hw_timer_t *timer = NULL; // Puntero al temporizador utilizado para la generación de timestamps

/**
 * Configura un temporizador por hardware para generar una interrupción
 * cada 1 ms, utilizada para incrementar un contador de timestamp en milisegundos.
 */
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

/**
 * Genera una secuencia de timestamps decrecientes a partir de un timestamp actual,
 * aplicando un incremento constante definido por `TIME_INCREMENT`.
 * @param buffer_timestamp Arreglo donde se almacenarán los timestamps generados.
 * @param num_samples Número total de muestras a generar.
 * @param actual_timestamp Timestamp base desde el cual se calcularán los anteriores.
 */
void generate_buffer_timestamp(uint32_t *buffer_timestamp, int num_samples, uint32_t actual_timestamp)
{
    for (int i = num_samples - 1; i >= 0; i--)
    {
        buffer_timestamp[i] = actual_timestamp - (num_samples - 1 - i) * TIME_INCREMENT; // Rellena los timestamps con un incremento
    }
}

/**
 * Tarea encargada de leer datos del acelerómetro ADXL355 y almacenarlos en la tarjeta SD.
 * Captura datos de FIFO, calcula los timestamps correspondientes y agrupa cada muestra
 * en bloques de 16 bytes (timestamp + aceleraciones X, Y, Z). Escribe los datos en
 * bloques temporales hasta que se alcanza el tamaño máximo, momento en el que los
 * guarda en un archivo binario nombrado por fecha (YYYYMMDD).
 */
void acelerometroTask(void *pvParameters)
{
    while (true)
    {
        if (isDataReady())
        {
            int fifoEntries = readRegistry(FIFO_ENTRIES);
            int samplesToRead = fifoEntries * 3;

            if (samplesToRead > 0 && samplesToRead % 9 == 0)
            {
                uint8_t *buffer = (uint8_t *)malloc(samplesToRead * sizeof(uint8_t));
                uint32_t *buffer_timestamp = (uint32_t *)malloc(samplesToRead / 9 * sizeof(uint32_t));

                generate_buffer_timestamp(buffer_timestamp, samplesToRead / 9, millis_timestamp);
                readFIFOData(buffer, samplesToRead);

                for (int i = 0; i < samplesToRead; i += 9)
                {
                    int32_t x_raw = raw_data(&buffer[i]);
                    int32_t y_raw = raw_data(&buffer[i + 3]);
                    int32_t z_raw = raw_data(&buffer[i + 6]);

                    if (x_raw == 0 && y_raw == 0 && z_raw == 0)
                        continue;

                    uint8_t dataToWrite[16]; // timestamp (4) + x (4) + y (4) + z (4)

                    // Copia los datos al buffer final
                    memcpy(dataToWrite, &buffer_timestamp[i / 9], sizeof(uint32_t));
                    memcpy(dataToWrite + 4, &x_raw, sizeof(int32_t));
                    memcpy(dataToWrite + 8, &y_raw, sizeof(int32_t));
                    memcpy(dataToWrite + 12, &z_raw, sizeof(int32_t));

                    if (bufferIndex + sizeof(dataToWrite) <= DATA_BLOCK_SIZE)
                    {
                        memcpy(&writeBuffer[bufferIndex], dataToWrite, sizeof(dataToWrite));
                        bufferIndex += sizeof(dataToWrite);
                    }
                    else
                    {
                        // Obtiene la hora actual del RTC
                        DateTime now = rtc.now();

                        // Convierte a formato YYYYMMDD
                        fecha_actual = now.year() * 10000 + now.month() * 100 + now.day();
                        if (fecha_actual != fecha_anterior)
                        {
                            fecha_anterior = fecha_actual;
                            // Crea nuevo nombre de archivo con la nueva fecha
                            sprintf(fileName, "/%08lu.bin", (unsigned long)fecha_actual);
                        }
                        writeToFile(fileName, writeBuffer, bufferIndex);
                        bufferIndex = 0;
                    }
                }

                free(buffer);
                free(buffer_timestamp);
            }
        }
    }
}

/**
 * Tarea periódica que verifica el estado de la conexión WiFi cada 10 segundos.
 * En caso de desconexión, intenta restablecer la conexión automáticamente.
 */
void wifi_task(void *parameter)
{
    while (true)
    {
        verificarWiFi();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * Tarea que mantiene activa la conexión MQTT ejecutando `client.loop()`
 * cada 100 ms, permitiendo la recepción de mensajes entrantes y el envío de keep-alive.
 */
void mqttLoopTask(void *parameter)
{
    while (true)
    {
        if (client.connected())
        {
            client.loop(); // 👈 Mantiene la conexión viva
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Ejecuta cada 100 ms aprox.
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
        fecha_actual = fecha;
        fecha_anterior = fecha;
        sprintf(fileName, "/%08lu.bin", (unsigned long)fecha_actual);

// Imprime los valores para depuración
#ifdef DEBUG
        Serial.printf("Hora RTC: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
        Serial.printf("Fecha RTC: %04d-%02d-%02d\n", now.year(), now.month(), now.day());
#endif
    }
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

    ///////////////////////////////////////// Wifi /////////////////////////////////////////////////////////////////////
    conectarWiFi();

    ///////////////////////////////////////// MQTT ///////////////////////////////////////////////////////
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    enviarEstado("on");
    enviarEstado("online");

    ///////////////////////////////////////////FreeRTOS//////////////////////////////////////////////////////////////////////

    // Crea las tareas para el acelerómetro y la sincronización
    xTaskCreatePinnedToCore(acelerometroTask, "AcelerometroTask", 8192, NULL, 2, &adx_taskHandle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(lectorTask, "lectorTask", 4096, NULL, 1, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(wifi_task, "WiFiTask", 4096, NULL, 1, &wifi_taskHandle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(mqttLoopTask, "MQTTLoopTask", 4096, NULL, 1, NULL, tskNO_AFFINITY);
}

void loop() {}
