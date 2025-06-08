#ifndef MQTT_MOD_H
#define MQTT_MOD_H
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configuración MQTT
constexpr const char *mqtt_server = ""; // Broker privado
constexpr const int mqtt_port = 1883;
constexpr const char *mqtt_user = ""; // Dejar vacío si no hay autenticación
constexpr const char *mqtt_pass = "";

extern WiFiClient espClient;
extern PubSubClient client;

// Crear el mensaje JSON LWT
constexpr const char *willTopic = "status";
constexpr const int willQos = 0;
constexpr const bool willRetain = false;

const String clientId = "ESP32_001";
const String lwtPayload = "{\"id\": \"" + clientId + "\", \"status\": \"offline\"}";
const String topic_status = "status";
const String topic_response = "events/" + clientId + "/response";

typedef struct
{
    bool lecturaActiva;
    bool error;
    uint32_t fecha;
    uint32_t timestampInicio;
    uint32_t timestampFin;
    uint32_t timestampOriginal;
    int32_t posicionInicial;
    int32_t offsetInicio;
    int32_t offsetFin;
} LecturaControl;

extern LecturaControl controlLectura;

// Prototipo de función
void reconnect();
void enviarEstado(const String &estado);
void callback(char *topic, byte *payload, unsigned int length);
uint32_t convertirFecha(uint32_t timestamp);
uint32_t convertirTiempoDelDia(uint32_t timestamp);
void enviarResultadoFinal(bool exito, uint32_t timestampOriginal);

#endif
