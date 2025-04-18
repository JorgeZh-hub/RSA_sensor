#include "Mqtt_mod.h"

WiFiClient espClient;
PubSubClient client(espClient);
DatosSensor datosSensor;

String clientId = "ESP32_001";
String lwtPayload = "{\"id\": \"" + clientId + "\", \"status\": \"offline\"}";

void enviarEstado(const String &estado)
{
    if (!client.connected())
    {
        reconnect();
    }

    DynamicJsonDocument doc(128);
    doc["id"] = "ESP32_001";
    doc["status"] = estado;

    char mensaje[128];
    serializeJson(doc, mensaje);

    if (client.publish("status", mensaje))
    {
        Serial.print("Mensaje enviado: ");
        Serial.println(mensaje);
    }
    else
    {
        Serial.println("Error al enviar el mensaje.");
    }
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Intentando conexión MQTT...");

        if (client.connect(
                clientId.c_str(),
                mqtt_user,
                mqtt_pass,
                willTopic,         // willTopic
                0,                 // willQoS
                true,              // willRetain
                lwtPayload.c_str() // willMessage
                ))
        {
            Serial.println("Conectado al broker MQTT!");

            // Enviar mensaje indicando que está online con el mismo formato
            String payloadOnline = "{\"id\": \"" + clientId + "\", \"status\": \"online\"}";
            client.publish(willTopic, payloadOnline.c_str(), true);
        }
        else
        {
            Serial.print("Error, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando en 5 segundos...");
        }
    }
}
