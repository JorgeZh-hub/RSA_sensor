#ifndef MQTT_MOD_H
#define MQTT_MOD_H
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configuración MQTT
constexpr const char *mqtt_server = "174.138.41.251"; // Broker público de prueba
constexpr const int mqtt_port = 1883;
constexpr const char *mqtt_user = "rsa"; // Dejar vacío si no hay autenticación
constexpr const char *mqtt_pass = "RSAiotace2023";

extern WiFiClient espClient;
extern PubSubClient client;

extern struct DatosSensor
{
    uint32_t fecha;     // 4 bytes
    uint32_t timestamp; // 4 bytes
    int32_t x_raw;      // 3 bytes
    int32_t y_raw;      // 3 bytes
    int32_t z_raw;      // 3 bytes
} datosSensor;

// Prototipo de función
void reconnect();
void enviarEstado(const String &estado);

#endif
