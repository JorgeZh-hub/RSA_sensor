#include "Mqtt_mod.h"

WiFiClient espClient;
PubSubClient client(espClient);
DatosSensor datosSensor;

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
        String clientId = "ESP32-" + String(random(0xffff), HEX);

        if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
        {
            Serial.println("Conectado al broker MQTT!");

            enviarEstado("online");
        }
        else
        {
            Serial.print("Error, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando en 5 segundos...");
            delay(5000);
        }
    }
}
