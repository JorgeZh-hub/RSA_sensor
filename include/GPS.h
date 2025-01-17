#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

// Constante para ajustar el día
constexpr bool ajuste_dia = false;

// Configuración de pines para el módulo GPS
#define GPS_TX 17 // Pin de transmisión del GPS (conectado al RX del ESP32)
#define GPS_RX 16 // Pin de recepción del GPS (conectado al TX del ESP32)

// Configuración de comunicación UART
#define GPS_BAUD 9600 // Velocidad de comunicación por UART (9600 bps, estándar para GPS)
#define UART_NUMBER 1 // Se utiliza UART1 para conectar el GPS al ESP32

// Comandos de configuración del GPS
static const char* gpsConfigCommands[] = {
    "$PMTK220,1000*1F",    // Tasa de actualización de 1000 ms
    "$PMTK313,1*2E",       // Activa el receptor de datos SBAS
    "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29", // Activa el mensaje GGA
    "$PMTK319,1*24",       // Activa el modo de bajo consumo
    "$PMTK413*34",         // Reinicio asistido del GPS
    "$PMTK513,1*28"        // Configura el modo de ahorro de energía avanzado
};

// Objeto Serial para la comunicación con el GPS
extern HardwareSerial GPS; // UART1 mapeado para GPS


// Prototipos de funciones usadas
void GPS_init(); // Inicializa la comunicación UART con el módulo GPS.
uint32_t RecuperarFechaGPS(const String &tramaDatosGPS); // Recupera la fecha a partir de una trama NMEA del GPS.
uint32_t RecuperarHoraGPS(const String &tramaDatosGPS); // Recupera la hora a partir de una trama NMEA del GPS.

#endif // GPS_H
