#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

constexpr bool ajuste_dia = false; // Constante de ajuste de día
#define GPS_TX 17 // Pin de transmisión (TX)
#define GPS_RX 16 // Pin de recepción (RX)
#define GPS_BAUD 9600 // Velocidad de comunicación UART
#define UART_NUMBER 1 

extern HardwareSerial GPS; // UART1 para el ESP32

// Function prototypes
void GPS_init();
uint32_t RecuperarFechaGPS(const String &tramaDatosGPS);
uint32_t RecuperarHoraGPS(const String &tramaDatosGPS);

#endif // GPS_H
