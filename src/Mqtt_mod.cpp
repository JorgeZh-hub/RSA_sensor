#include "Mqtt_mod.h"
#include "SD.h"

WiFiClient espClient;
PubSubClient client(espClient);
DatosSensor datosSensor;

LecturaControl controlLectura = {false,false, 0, 0, 0, 0, 0, 0, 0};


uint32_t convertirFecha(uint32_t timestamp)
{
    time_t rawTime = timestamp;
    struct tm *timeinfo = gmtime(&rawTime);

    uint32_t fecha = (timeinfo->tm_year + 1900) * 10000 +
                     (timeinfo->tm_mon + 1) * 100 +
                     timeinfo->tm_mday;

    return fecha;
}

uint32_t convertirTiempoDelDia(uint32_t timestamp)
{
    time_t rawTime = timestamp;
    struct tm *timeinfo = gmtime(&rawTime);

    uint32_t segundosDelDia = timeinfo->tm_hour * 3600 +
                              timeinfo->tm_min * 60 +
                              timeinfo->tm_sec;

    return segundosDelDia * 1000; // lo guardas en milisegundos
}

void enviarEstado(const String &estado)
{
    if (!client.connected())
    {
        reconnect();
    }

    DynamicJsonDocument doc(128);
    doc["id"] = clientId;
    doc["status"] = estado;

    char mensaje[128];
    serializeJson(doc, mensaje);

    if (client.publish(topic_status.c_str(), mensaje))
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

            client.subscribe("events/ESP32_001/request");
            Serial.println("Suscrito al tópico: events/ESP32_001/request");
        }
        else
        {
            Serial.print("Error, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando en 5 segundos...");
        }
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Mensaje recibido en el tópico: ");
    Serial.println(topic);

    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("Contenido del mensaje: ");
    Serial.println(message);

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        Serial.println("Error al parsear JSON.");
        return;
    }

    String id = doc["id"];
    String timestamp = doc["timestamp"];
    int duration = doc["duration"];

    if (id == clientId)
    {
        Serial.println("Solicitud válida recibida para este dispositivo.");
        Serial.print("Timestamp: ");
        Serial.println(timestamp);
        Serial.print("Duración: ");
        Serial.println(duration);

        // Envío de Received
        DynamicJsonDocument doc(128);
        doc["id"] = clientId;
        doc["timestamp"] = timestamp;
        doc["status"] = "received";

        char mensaje[128];
        serializeJson(doc, mensaje);

        client.publish(topic_response.c_str(), mensaje);
        Serial.println("Mensaje received enviado: ");

        // Envío de Processing
        doc["status"] = "processing";

        serializeJson(doc, mensaje);

        client.publish(topic_response.c_str(), mensaje);
        Serial.println("Mensaje processing enviado: ");

        uint32_t tsUnix = timestamp.toInt();
        controlLectura.fecha = convertirFecha(tsUnix);
        controlLectura.timestampInicio = convertirTiempoDelDia(tsUnix);
        controlLectura.timestampFin = controlLectura.timestampInicio + duration;
        controlLectura.posicionInicial = 0; // Empieza desde el inicio (o mejora con índice si tienes uno)
        controlLectura.lecturaActiva = true;
        controlLectura.timestampOriginal = (uint32_t) timestamp.toInt();
        controlLectura.error = false;                 // Reinicia bandera de error
    }
    else
    {
        Serial.println("ID no coincide. Ignorando mensaje.");
    }
}
