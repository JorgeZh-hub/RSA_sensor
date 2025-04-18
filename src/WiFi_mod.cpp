#include "WiFi_mod.h"
#include "Mqtt_mod.h"

void conectarWiFi()
{
    Serial.print("Conectando a WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 20)
    { // Intentar por 20 ciclos
        vTaskDelay(pdMS_TO_TICKS(10000));
        Serial.print(".");
        intentos++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nConectado exitosamente");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nNo se pudo conectar a la red");
    }
}

void verificarWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nWiFi desconectado. Reconectando...");
        conectarWiFi();
        reconnect();
    }
    else
    {
        Serial.println("WiFi conectado y funcionando.");
    }
}
