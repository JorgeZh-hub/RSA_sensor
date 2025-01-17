#include "GPS.h"

// Define la instancia de HardwareSerial
HardwareSerial GPS(UART_NUMBER);

/**
 * Inicializa la comunicación UART con el módulo GPS.
 */
void GPS_init() {
    #ifdef DEBUG
        Serial.println("Inicializando GPS...");
    #endif

    // Configuración de la UART para el módulo GPS
    GPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX); // Configura UART1 con 9600 bps, 8 bits de datos, sin paridad, 1 bit de parada

    // Envía los comandos de configuración específicos para el módulo GPS
    for (const char* command : gpsConfigCommands) {
        GPS.println(command); // Envía el comando al módulo
        #ifdef DEBUG
            Serial.println(String("Enviando comando: ") + command); // Imprime en consola para depuración
        #endif
        delay(100); // Espera un breve momento para procesar cada comando
    }

    #ifdef DEBUG
        Serial.println("Configuración del GPS completa.");
    #endif

    delay(1000); // Espera adicional para garantizar la configuración del módulo
}

/**
 * Recupera la fecha a partir de una trama NMEA del GPS.
 * @param tramaDatosGPS Cadena de datos (trama NMEA) recibida del GPS.
 * @return Fecha en formato entero (YYYYMMDD).
 */
uint32_t RecuperarFechaGPS(const String &tramaDatosGPS) {
    int fechaIdx = 0;
    int comaCount = 0;

    // Encuentra la posición de la novena coma en la trama (donde se encuentra la fecha)
    for (int i = 0; i < tramaDatosGPS.length(); i++) {
        if (tramaDatosGPS[i] == ',') comaCount++;
        if (comaCount == 9) {
            fechaIdx = i + 1;
            break;
        }
    }

    // Verifica que la posición encontrada sea válida y que los datos sean suficientes
    if (fechaIdx == 0 || fechaIdx + 6 > tramaDatosGPS.length()) return 0;

    // Extrae la fecha en formato DDMMYY
    String fechaStr = tramaDatosGPS.substring(fechaIdx, fechaIdx + 6);
    uint32_t dia = fechaStr.substring(0, 2).toInt();
    uint32_t mes = fechaStr.substring(2, 4).toInt();
    uint32_t anio = fechaStr.substring(4, 6).toInt() + 2000; // Ajusta el año al formato completo

    // Devuelve la fecha en formato YYYYMMDD
    return (anio * 10000) + (mes * 100) + dia;
}

/**
 * Recupera la hora a partir de una trama NMEA del GPS.
 * @param tramaDatosGPS Cadena de datos (trama NMEA) recibida del GPS.
 * @return Hora en formato entero (HHMMSS).
 */
uint32_t RecuperarHoraGPS(const String &tramaDatosGPS) {
    // Encuentra la posición de la primera coma (donde empieza la hora UTC)
    int horaIdx = tramaDatosGPS.indexOf(',') + 1;

    // Verifica que la posición encontrada sea válida y que los datos sean suficientes
    if (horaIdx < 0 || tramaDatosGPS.length() < horaIdx + 6) return 0;

    // Extrae la hora en formato HHMMSS
    String horaStr = tramaDatosGPS.substring(horaIdx, horaIdx + 6);
    int horas = horaStr.substring(0, 2).toInt();
    int minutos = horaStr.substring(2, 4).toInt();
    int segundos = horaStr.substring(4, 6).toInt();

    // Ajuste de zona horaria (UTC-5, puede configurarse según la necesidad)
    horas -= 5;
    if (horas < 0) horas += 24; // Ajusta si la hora es negativa (cambio de día)

    // Devuelve la hora en segundos desde medianoche
    return (horas * 3600) + (minutos * 60) + segundos;
}

