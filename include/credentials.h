
#ifndef CREDENTIALS
#define CREDENTIALS
#include <Arduino.h>


// Configuración MQTT
constexpr const char *mqtt_server = "10.26.23.64"; // Broker privado
constexpr const int mqtt_port = 1883;
constexpr const char *mqtt_user = ""; // Dejar vacío si no hay autenticación
constexpr const char *mqtt_pass = "";


// Credenciales SSID
constexpr const char *ssid = "Red abierta UCUENCA";
constexpr const char *password = "";

#endif