#ifndef RTC3231_H
#define RTC3231_H

#define DEBUG // Define la macro para habilitar mensajes de depuración

#include <Arduino.h>
#include <Wire.h>   // Biblioteca para comunicación I2C
#include <RTClib.h> // Biblioteca para trabajar con el RTC DS3231
#include <time.h>   // Biblioteca para manejo de tiempo (NTP)


const uint32_t SERIAL_SPEED = 115200; // Velocidad en baudios para la comunicación serial
const uint32_t I2C_SDA = 21; // SDA de I2C
const uint32_t I2C_SCL = 22; // SCL de I2C
const uint32_t SQW_INT = 34; // Pin de interrupción para la sincronización con el RTC

// Objeto global para interactuar con el RTC DS3231
extern RTC_DS3231 rtc;

// Configuración del servidor NTP
constexpr const char *ntpServer = "pool.ntp.org"; // Servidor NTP para sincronización
constexpr long gmtOffset_sec = -5 * 3600;         // Desfase de tiempo para GMT-5 (UTC-5)
constexpr int daylightOffset_sec = 0;             // No se aplica cambio de horario de verano

// Variables globales
extern volatile uint32_t millis_timestamp; // Timestamp en milisegundos (actualizado regularmente)
extern uint32_t hora;                      // Variable para almacenar la hora actual
extern uint32_t fecha;                     // Variable para almacenar la fecha actual

// Prototipos de funciones
void updateRTCFromNTP(); // Sincroniza el reloj RTC con la hora obtenida desde un servidor NTP.
void updateRTCfromGPS(); // Sincroniza el reloj RTC con la hora obtenida desde un módulo GPS.

#endif // RTC3231_H
