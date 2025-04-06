#include <SPI.h>
#include "ADXL355.h"

// Inicialización de la clase SPI para el bus HSPI
SPIClass spiADXL(HSPI); // Se utiliza HSPI, asegurando que no interfiera con VSPI si se usa otro hardware SPI

/**
 * Escribe un valor en un registro del ADXL355.
 * @param thisRegister Dirección del registro a escribir.
 * @param thisValue Valor a escribir en el registro.
 */
void writeRegister(byte thisRegister, byte thisValue)
{
  // Preparación del byte de comando para escritura (R/W bit en 0)
  byte dataToSend = (thisRegister << 1) | WRITE_BYTE;

  // Inicio de la comunicación SPI
  digitalWrite(ADXL_CS, LOW);
  spiADXL.transfer(dataToSend); // Envía la dirección del registro
  spiADXL.transfer(thisValue); // Envía el valor a escribir
  digitalWrite(ADXL_CS, HIGH); // Finaliza la comunicación
}

/**
 * Lee un valor de un registro del ADXL355.
 * @param thisRegister Dirección del registro a leer.
 * @return Valor leído (8 bits).
 */
unsigned int readRegistry(byte thisRegister)
{
  unsigned int result = 0;
  // Preparación del byte de comando para lectura (R/W bit en 1)
  byte dataToSend = (thisRegister << 1) | READ_BYTE;

  // Inicio de la comunicación SPI
  digitalWrite(ADXL_CS, LOW);
  spiADXL.transfer(dataToSend);    // Envía la dirección del registro
  result = spiADXL.transfer(0x00); // Recibe el valor del registro
  digitalWrite(ADXL_CS, HIGH);     // Finaliza la comunicación

  return result;
}

/**
 * Convierte datos crudos del acelerómetro a una medida de aceleración.
 * @param data Puntero a los 3 bytes de datos crudos (X, Y o Z).
 * @return Valor de aceleración en g.
 */
float convert_data(uint8_t *data)
{
  // Convierte los 20 bits de datos crudos a un entero con signo
  int32_t raw = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (int32_t)data[2] >> 4;

  // Ajusta el valor si el bit de signo está activado (complemento a 2)
  if (raw & 0x80000)
    raw -= 0x100000;

  // Convierte el valor crudo a g (según la escala de sensibilidad del sensor)
  float acceleration = raw * 0.00374; // Escala en g/LSB (puede cambiar según el rango configurado)
  return acceleration;
}

/**
 * Convierte datos crudos en un valor entero de 32 bits.
 * @param data Puntero a los 3 bytes de datos crudos (X, Y o Z).
 * @return Valor entero convertido.
 */
int32_t raw_data(uint8_t *data)
{
  int32_t raw = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (int32_t)data[2] >> 4;

  // Ajusta el valor si el bit de signo está activado
  if (raw & 0x80000)
    raw -= 0x100000;

  return raw;
}

/**
 * Lee datos del FIFO del ADXL355.
 * @param buffer Puntero al buffer donde se almacenarán los datos leídos.
 * @param length Número de bytes a leer del FIFO.
 */
void readFIFOData(uint8_t *buffer, int length)
{
  digitalWrite(ADXL_CS, LOW);
  byte dataToSend = (FIFO_DATA << 1) | READ_BYTE;
  spiADXL.transfer(dataToSend); // Envía el comando para leer datos del FIFO

  // Lee los bytes solicitados del FIFO
  for (int i = 0; i < length; i++)
  {
    buffer[i] = spiADXL.transfer(0x00);
  }
  digitalWrite(ADXL_CS, HIGH);
}

/**
 * Verifica si hay datos nuevos listos en el sensor.
 * @return Verdadero si hay datos listos, falso en caso contrario.
 */
bool isDataReady()
{
  uint8_t status = readRegistry(STATUS_REG);
  return (status & 0x01); // Verifica el bit DTA_RDY
}
