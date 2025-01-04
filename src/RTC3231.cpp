#include "RTC3231.h"
#include "GPS.h"
RTC_DS3231 rtc;

volatile uint32_t millis_timestamp = 0;  // Definición de la variable global
uint32_t hora = 0;                    // Definición de la fecha
uint32_t fecha = 0;                    // Definición de la fecha

void updateRTCfromGPS() {
    #ifdef DEBUG
        Serial.print("Updating RTC from GPS...");
    #endif
    while (true) {
        if (GPS.available()) {
            String tramaGPS = GPS.readStringUntil('\n');
            if (tramaGPS.startsWith("$GPRMC")) {
                #ifdef DEBUG
                Serial.print(".");
                #endif
                int comaIdx1 = tramaGPS.indexOf(',');
                int comaIdx2 = tramaGPS.indexOf(',', comaIdx1 + 1);
                int comaIdx3 = tramaGPS.indexOf(',', comaIdx2 + 1);

                if (comaIdx2 > comaIdx1 && comaIdx3 > comaIdx2 + 1) {
                    char validez = tramaGPS.charAt(comaIdx2 + 1);
                    if (validez == 'A') {
                        hora = RecuperarHoraGPS(tramaGPS);
                        fecha = RecuperarFechaGPS(tramaGPS);

                        millis_timestamp = hora * 1000; // Convertir a milisegundos


                        // Extraer valores individuales de fecha
                        unsigned int anio = fecha / 10000;  // aaaa
                        unsigned int mes = (fecha / 100) % 100; // mm
                        unsigned int dia = fecha % 100; // dd

                        // Extraer valores individuales de hora
                        unsigned int horas = hora / 3600; // hh
                        unsigned int minutos = (hora / 60) % 60; // mm
                        unsigned int segundos = hora % 60; // ss

                        if (horas >= 19){
                            dia--;       // Ajuste de día
                        }

                        // Ajustar el RTC con los valores obtenidos
                        rtc.adjust(DateTime(anio, mes, dia, horas, minutos, segundos));


                        #ifdef DEBUG
                            Serial.println("RTC updated from GPS:");
                            Serial.println("Fecha GPS: " + String(anio) + "-" + String(mes) + "-" + String(dia));
                            Serial.println("Hora GPS: " + String(horas) + ":" + String(minutos) + ":" + String(segundos));
                        #endif
                        break;
                    }
                }
            }
        }
    }
}


void updateRTCFromNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(1000);
        now = time(nullptr);
    }

    time(&now);
    struct tm *local_tm = localtime(&now);
    if (local_tm != NULL) {
        rtc.adjust(DateTime(local_tm->tm_year + 1900, local_tm->tm_mon + 1, local_tm->tm_mday, 
                            local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec));
        #ifdef DEBUG
            Serial.println("RTC updated from NTP");
        #endif
    } else {
        #ifdef DEBUG
        Serial.println("Failed to get time");
        #endif
    }
}