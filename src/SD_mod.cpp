#include "SD_mod.h" // Inclusión del archivo de encabezado para manejar la tarjeta SD

SPIClass spiSD(VSPI); // Se crea una instancia de SPIClass para la comunicación con el módulo SD usando el bus SPI (VSPI)

// Declaración de variables globales para manejar el flujo de datos de la SD
volatile bool sdWriteFlag = false;                 // Bandera que indica si hay datos listos para escribir en la SD
portMUX_TYPE sdMux = portMUX_INITIALIZER_UNLOCKED; // Mutex para proteger el acceso a la bandera en entornos multitarea (prevenir condiciones de carrera)
size_t bufferIndex = 0;                            // Índice para la posición actual de los datos en el buffer
size_t mybufferIndex = 0;                          // Índice auxiliar para controlar la posición en otro buffer
byte writeBuffer[DATA_BLOCK_SIZE];                 // Buffer de tamaño definido que contiene los datos a escribir en la SD
byte myBuffer[DATA_BLOCK_SIZE];                    // Otro buffer auxiliar para manejar los datos

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
