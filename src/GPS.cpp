#include "GPS.h"

// Define la instancia de HardwareSerial
HardwareSerial GPS(UART_NUMBER);

// Comandos de configuración del GPS (definidos solo en GPS.cpp)
static const char* gpsConfigCommands[] = {
    "$PMTK220,1000*1F",    // Tasa de actualización de 1000 ms
    "$PMTK313,1*2E",       // Activa el receptor de datos SBAS
    "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29", // Activa el mensaje GGA
    "$PMTK319,1*24",       // Activa el modo de bajo consumo
    "$PMTK413*34",         // Reinicio asistido del GPS
    "$PMTK513,1*28"        // Configura el modo de ahorro de energía avanzado
};

// Inicialización del GPS
void GPS_init() {
    #ifdef DEBUG
        Serial.println("Inicializando GPS...");
    #endif
    GPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX); // Configura UART1

    // Envía los comandos de configuración
    for (const char* command : gpsConfigCommands) {
        GPS.println(command); // Envía el comando al GPS
        #ifdef DEBUG
            Serial.println(String("Enviando comando: ") + command);
        #endif
        delay(100);
    }
    #ifdef DEBUG
    Serial.println("Configuración del GPS completa.");
    #endif
    delay(1000); // Espera adicional para completar la configuración
}

// Funciones para recuperar la hora y fecha
uint32_t RecuperarFechaGPS(const String &tramaDatosGPS) {
    int fechaIdx = 0;
    int comaCount = 0;
    for (int i = 0; i < tramaDatosGPS.length(); i++) {
        if (tramaDatosGPS[i] == ',') comaCount++;
        if (comaCount == 9) {
            fechaIdx = i + 1;
            break;
        }
    }
    if (fechaIdx == 0 || fechaIdx + 6 > tramaDatosGPS.length()) return 0;

    String fechaStr = tramaDatosGPS.substring(fechaIdx, fechaIdx + 6);
    uint32_t dia = fechaStr.substring(0, 2).toInt();
    uint32_t mes = fechaStr.substring(2, 4).toInt();
    uint32_t anio = fechaStr.substring(4, 6).toInt() + 2000;

    return (anio * 10000) + (mes * 100) + dia;
}

uint32_t RecuperarHoraGPS(const String &tramaDatosGPS) {
    int horaIdx = tramaDatosGPS.indexOf(',') + 1;
    if (horaIdx < 0 || tramaDatosGPS.length() < horaIdx + 6) return 0;

    String horaStr = tramaDatosGPS.substring(horaIdx, horaIdx + 6);
    int horas = horaStr.substring(0, 2).toInt();
    int minutos = horaStr.substring(2, 4).toInt();
    int segundos = horaStr.substring(4, 6).toInt();

    horas -= 5;                                                             // Ajuste de zona horaria UTC-5
    if (horas < 0) horas += 24;                                             // Ajuste de día

    return (horas * 3600) + (minutos * 60) + segundos;                      // Devuelve la hora en segundos
}
