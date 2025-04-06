#include "RTC3231.h"
#include "GPS.h"

// Instancia global del RTC DS3231
RTC_DS3231 rtc;

// Variables globales para timestamp, hora y fecha
volatile uint32_t millis_timestamp = 0;  // Timestamp en milisegundos
uint32_t hora = 0;                       // Variable para almacenar la hora actual
uint32_t fecha = 0;                      // Variable para almacenar la fecha actual

/**
 * Actualiza el RTC utilizando la hora y fecha obtenidas desde un módulo GPS.
 */
void updateRTCfromGPS() {
    #ifdef DEBUG
        Serial.print("Updating RTC from GPS...");
    #endif

    while (true) {
        // Revisa si hay datos disponibles desde el GPS
        if (GPS.available()) {
            // Lee una línea completa de datos desde el GPS
            String tramaGPS = GPS.readStringUntil('\n');

            // Filtra solo las tramas RMC que contienen información de tiempo y posición
            if (tramaGPS.startsWith("$GPRMC")) {
                #ifdef DEBUG
                Serial.print(".");
                #endif

                // Encuentra las posiciones de las comas para extraer datos específicos
                int comaIdx1 = tramaGPS.indexOf(',');
                int comaIdx2 = tramaGPS.indexOf(',', comaIdx1 + 1);
                int comaIdx3 = tramaGPS.indexOf(',', comaIdx2 + 1);

                // Verifica si la trama es válida
                if (comaIdx2 > comaIdx1 && comaIdx3 > comaIdx2 + 1) {
                    char validez = tramaGPS.charAt(comaIdx2 + 1); // Obtiene el estado de validez ('A' para válida)
                    if (validez == 'A') { // Solo procede si la trama es válida
                        hora = RecuperarHoraGPS(tramaGPS); // Extrae la hora del GPS
                        fecha = RecuperarFechaGPS(tramaGPS); // Extrae la fecha del GPS

                        millis_timestamp = hora * 1000; // Convierte la hora a milisegundos

                        // Descompone la fecha en año, mes y día
                        unsigned int anio = fecha / 10000;        // aaaa
                        unsigned int mes = (fecha / 100) % 100;   // mm
                        unsigned int dia = fecha % 100;          // dd

                        // Descompone la hora en horas, minutos y segundos
                        unsigned int horas = hora / 3600;        // hh
                        unsigned int minutos = (hora / 60) % 60; // mm
                        unsigned int segundos = hora % 60;       // ss

                        // Ajuste de día si las horas indican un cambio de fecha
                        if (horas >= 19) {
                            dia--; // Ajusta el día para reflejar el cambio
                        }

                        // Configura el RTC con los valores obtenidos
                        rtc.adjust(DateTime(anio, mes, dia, horas, minutos, segundos));

                        // Mensajes de depuración para confirmar la actualización
                        #ifdef DEBUG
                            Serial.println("RTC updated from GPS:");
                            Serial.println("Fecha GPS: " + String(anio) + "-" + String(mes) + "-" + String(dia));
                            Serial.println("Hora GPS: " + String(horas) + ":" + String(minutos) + ":" + String(segundos));
                        #endif
                        break; // Sale del bucle una vez actualizado el RTC
                    }
                }
            }
        }
    }
}

/**
 * Actualiza el RTC utilizando la hora obtenida desde un servidor NTP.
 */
void updateRTCFromNTP() {
    // Configura el tiempo con el servidor NTP y los ajustes de zona horaria
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    time_t now = time(nullptr); // Obtiene la hora actual desde el servidor NTP

    // Espera hasta que se obtenga un tiempo válido
    while (now < 8 * 3600 * 2) { // Verifica que la hora sea razonable (no valores predeterminados)
        delay(1000); // Espera 1 segundo antes de volver a intentarlo
        now = time(nullptr);
    }

    // Verifica si se obtuvo un tiempo válido
    time(&now);
    struct tm *local_tm = localtime(&now);
    if (local_tm != NULL) {
        // Configura el RTC con los valores obtenidos desde NTP
        rtc.adjust(DateTime(local_tm->tm_year + 1900, local_tm->tm_mon + 1, local_tm->tm_mday, 
                            local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec));
        #ifdef DEBUG
            Serial.println("RTC updated from NTP"); // Confirma la actualización
        #endif
    } else {
        // Muestra un mensaje de error si no se pudo obtener la hora
        #ifdef DEBUG
        Serial.println("Failed to get time");
        #endif
    }
}
