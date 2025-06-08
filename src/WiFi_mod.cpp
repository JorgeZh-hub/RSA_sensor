#include "WiFi_mod.h"
#include "Mqtt_mod.h"

/**
 * Establece la conexión WiFi utilizando el SSID y contraseña definidos.
 * Realiza hasta 20 intentos con retardo entre cada uno. Muestra la IP
 * local si se conecta correctamente, o un mensaje de error en caso contrario.
 */
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

/**
 * Verifica el estado de la conexión WiFi. Si está desconectada,
 * intenta reconectar a la red y restablecer la conexión MQTT.
 */
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
