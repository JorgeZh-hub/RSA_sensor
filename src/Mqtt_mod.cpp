#include "Mqtt_mod.h"

WiFiClient espClient;
PubSubClient client(espClient);
DatosSensor datosSensor;

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Intentando conexión MQTT...");
        String clientId = "ESP32-" + String(random(0xffff), HEX);

        if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
        {
            Serial.println("Conectado al broker MQTT!");
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