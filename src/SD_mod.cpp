#include "SD_mod.h" // Inclusión del archivo de encabezado para manejar la tarjeta SD
#include "Mqtt_mod.h"

SPIClass spiSD(VSPI); // Se crea una instancia de SPIClass para la comunicación con el módulo SD usando el bus SPI (VSPI)

// Declaración de variables globales para manejar el flujo de datos de la SD
// volatile bool sdWriteFlag = false;                 // Bandera que indica si hay datos listos para escribir en la SD
// portMUX_TYPE sdMux = portMUX_INITIALIZER_UNLOCKED; // Mutex para proteger el acceso a la bandera en entornos multitarea (prevenir condiciones de carrera)
size_t bufferIndex = 0; // Índice para la posición actual de los datos en el buffer
// size_t mybufferIndex = 0;                          // Índice auxiliar para controlar la posición en otro buffer
byte writeBuffer[DATA_BLOCK_SIZE]; // Buffer de tamaño definido que contiene los datos a escribir en la SD
// byte myBuffer[DATA_BLOCK_SIZE];                    // Otro buffer auxiliar para manejar los datos
bool writting = false;
SemaphoreHandle_t sdMutex = xSemaphoreCreateMutex(); // Definición del mutex
char file_name[20] = "/00000000.bin";                // Nombre actual del archivo
int32_t timestamp, x, y, z;

int32_t buscarOffsetInicio(File &file, uint32_t timestampObjetivo)
{
    const size_t blockSize = 16;
    uint8_t buffer[blockSize];

    size_t totalSize = file.size();
    int32_t left = 0;
    int32_t right = totalSize / blockSize - 1;
    int32_t result = -1;

    while (left <= right)
    {
        int32_t mid = left + (right - left) / 2;
        size_t offset = mid * blockSize;

        file.seek(offset);
        if (file.read(buffer, blockSize) != blockSize)
        {
            Serial.println("❌ Error al leer durante búsqueda binaria.");
            return -1;
        }

        uint32_t ts;
        memcpy(&ts, buffer, 4); // timestamp está al inicio del bloque

        if (ts < timestampObjetivo)
        {
            left = mid + 1;
        }
        else
        {
            result = offset;
            right = mid - 1;
        }
    }

    return result;
}

int32_t buscarOffsetFin(File &file, uint32_t timestampObjetivo)
{
    const size_t blockSize = 16;
    uint8_t buffer[blockSize];

    size_t totalSize = file.size();
    int32_t left = 0;
    int32_t right = totalSize / blockSize - 1;
    int32_t result = -1;

    while (left <= right)
    {
        int32_t mid = left + (right - left) / 2;
        size_t offset = mid * blockSize;

        file.seek(offset);
        if (file.read(buffer, blockSize) != blockSize)
        {
            Serial.println("❌ Error al leer durante búsqueda binaria.");
            return -1;
        }

        uint32_t ts;
        memcpy(&ts, buffer, 4); // timestamp está al inicio

        if (ts <= timestampObjetivo)
        {
            result = offset;
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return result;
}

// Función para escribir datos en un archivo en la tarjeta SD
void writeToFile(const char *filename, const byte *buffer, size_t bufferSize)
{
    File file = SD.open(filename, FILE_APPEND); // Abre el archivo en modo de añadir (append)
    if (file)
    {
        file.write(buffer, bufferSize); // Escribe los datos del buffer en el archivo
        file.close();                   // Cierra el archivo después de escribir
    }
    else
    {
#ifdef DEBUG
        Serial.println("SD Write Failed"); // Imprime un mensaje de error si la escritura falla
#endif
    }
}

void lectorTask(void *pvParameters)
{
    const size_t blockSize = 16;
    const int bloquesPorIteracion = 10;
    uint8_t buffer[blockSize];

    while (true)
    {
        if (controlLectura.lecturaActiva)
        {
            if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(50)))
            {
                sprintf(file_name, "/%08lu.bin", (unsigned long)controlLectura.fecha); // Formatea el nombre del archivo con la fecha
                File file = SD.open(file_name, FILE_READ);
                if (file)
                {
                    // Inicializar límites si aún no están definidos
                    if (controlLectura.posicionInicial == 0)
                    {
                        int32_t offsetInicio = buscarOffsetInicio(file, controlLectura.timestampInicio);
                        int32_t offsetFin = buscarOffsetFin(file, controlLectura.timestampFin);

                        Serial.printf("✅ Intervalo solicitado: inicio = %d, fin = %d\n", controlLectura.timestampInicio, controlLectura.timestampFin);

                        if (offsetInicio >= 0 && offsetFin >= 0 && offsetInicio <= offsetFin)
                        {
                            controlLectura.offsetInicio = offsetInicio;
                            controlLectura.offsetFin = offsetFin;
                            controlLectura.posicionInicial = offsetInicio;

                            Serial.printf("✅ Intervalo encontrado: inicio = %d, fin = %d\n", offsetInicio, offsetFin);
                        }
                        else
                        {
                            Serial.println("❌ No se encontró un intervalo válido.");
                            controlLectura.lecturaActiva = false;
                            controlLectura.error = true;
                            file.close();
                            xSemaphoreGive(sdMutex);
                            enviarResultadoFinal(false, controlLectura.timestampOriginal);
                            continue;
                        }
                    }

                    int bloquesProcesados = 0;
                    bool finLectura = false;

                    while (controlLectura.posicionInicial <= controlLectura.offsetFin && bloquesProcesados < bloquesPorIteracion)
                    {
                        file.seek(controlLectura.posicionInicial);
                        if (file.read(buffer, blockSize) != blockSize)
                        {
                            Serial.println("❌ Error al leer bloque.");
                            controlLectura.error = true;
                            controlLectura.lecturaActiva = false;
                            break;
                        }

                        controlLectura.posicionInicial += blockSize;
                        bloquesProcesados++;

                        uint32_t timestampRegistro;
                        memcpy(&timestampRegistro, buffer, 4);

                        // 📌 Imprimir información
                        Serial.printf("📦 Bloque %d - Offset: %d\n", bloquesProcesados, controlLectura.posicionInicial - blockSize);
                        Serial.printf("Fecha: %u, Timestamp: %u\n", controlLectura.fecha, timestampRegistro);

                        if (timestampRegistro < controlLectura.timestampInicio)
                        {
                            Serial.println("↪ Timestamp menor al inicio, bloque descartado.");
                            continue;
                        }

                        if (timestampRegistro > controlLectura.timestampFin)
                        {
                            Serial.println("↪ Timestamp mayor al fin, terminando lectura.");
                            controlLectura.lecturaActiva = false;
                            finLectura = true;
                            break;
                        }

                        if (client.publish("events/ESP32_001/data", buffer, blockSize))
                        {
                            Serial.println("✅ Registro enviado.");
                            memcpy(&timestamp, buffer, 4);
                            memcpy(&x, buffer + 4, 4);
                            memcpy(&y, buffer + 8, 4);
                            memcpy(&z, buffer + 12, 4);

                            Serial.printf("📤 Enviando -> Timestamp: %ld ms | X: %ld | Y: %ld | Z: %ld (cm/s²)\n", timestamp, x, y, z);
                        }
                        else
                        {
                            Serial.println("❌ Error al publicar registro.");
                            controlLectura.error = true;
                            controlLectura.lecturaActiva = false;
                            finLectura = true;
                            break;
                        }

                        vTaskDelay(5 / portTICK_PERIOD_MS);
                    }
                    // 👉 Verifica si ya terminaste de leer todos los bloques
                    if (controlLectura.posicionInicial > controlLectura.offsetFin && !controlLectura.error)
                    {
                        controlLectura.lecturaActiva = false;
                        finLectura = true;
                    }

                    file.close();
                    xSemaphoreGive(sdMutex);

                    if (!controlLectura.lecturaActiva && !controlLectura.error && finLectura)
                    {
                        enviarResultadoFinal(true, controlLectura.timestampOriginal);
                    }
                    else if (controlLectura.error)
                    {
                        enviarResultadoFinal(false, controlLectura.timestampOriginal);
                    }
                }
                else
                {
                    xSemaphoreGive(sdMutex);
                    Serial.println("❌ Error al abrir el archivo.");
                    controlLectura.error = true;
                    controlLectura.lecturaActiva = false;
                }
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
