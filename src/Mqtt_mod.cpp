#include "Mqtt_mod.h"
#include "SD.h"

WiFiClient espClient;
PubSubClient client(espClient);

LecturaControl controlLectura = {false, false, 0, 0, 0, 0, 0, 0, 0};

/**
 * Convierte un timestamp UNIX a una fecha en formato YYYYMMDD ajustada a la zona horaria GMT-5.
 * @param timestamp Timestamp en segundos desde época UNIX.
 * @return Fecha ajustada en formato entero YYYYMMDD.
 */
uint32_t convertirFecha(uint32_t timestamp)
{
    // Ajustar el timestamp a GMT-5
    time_t rawTime = timestamp - 5 * 3600;
    struct tm *timeinfo = gmtime(&rawTime);

    uint32_t fecha = (timeinfo->tm_year + 1900) * 10000 +
                     (timeinfo->tm_mon + 1) * 100 +
                     timeinfo->tm_mday;

    return fecha;
}

/**
 * Convierte un timestamp UNIX a milisegundos transcurridos desde medianoche,
 * ajustado a la zona horaria GMT-5.
 * @param timestamp Timestamp en segundos desde época UNIX.
 * @return Milisegundos del día (0–86399999).
 */
uint32_t convertirTiempoDelDia(uint32_t timestamp)
{
    // Ajustar a GMT-5
    time_t rawTime = timestamp - 5 * 3600;

    // Obtener componentes de tiempo UTC (ajustado)
    struct tm *timeinfo = gmtime(&rawTime);

    // Calcular milisegundos desde medianoche
    uint32_t segundosDelDia = timeinfo->tm_hour * 3600 +
                              timeinfo->tm_min * 60 +
                              timeinfo->tm_sec;

    return segundosDelDia * 1000;
}

/**
 * Publica un mensaje de estado en formato JSON al tópico MQTT correspondiente.
 * Si la conexión con el broker no está activa, intenta reconectarse primero.
 * @param estado Estado a reportar (por ejemplo, "online", "offline", etc.).
 */
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

/**
 * Envía un mensaje JSON final indicando el resultado de una operación.
 * Incluye el ID del dispositivo, el timestamp original y el estado ("finished" o "error").
 * Se publica en el tópico MQTT de respuesta.
 * @param exito Indica si la operación fue exitosa.
 * @param timestampOriginal Timestamp original asociado a la operación.
 */
void enviarResultadoFinal(bool exito, uint32_t timestampOriginal)
{
    if (!client.connected())
    {
        reconnect();
    }

    DynamicJsonDocument doc(128);
    doc["id"] = clientId;
    doc["timestamp"] = timestampOriginal;
    doc["status"] = exito ? "finished" : "error";

    char mensaje[128];
    serializeJson(doc, mensaje);

    client.publish(topic_response.c_str(), mensaje);
    Serial.print("Mensaje final enviado: ");
    Serial.println(mensaje);
}

/**
 * Intenta reconectarse al broker MQTT si la conexión se ha perdido.
 * Publica un mensaje de estado "online" y se suscribe al tópico de solicitudes.
 * Reintenta indefinidamente hasta lograr la conexión.
 */
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

/**
 * Callback que procesa los mensajes MQTT recibidos en formato JSON.
 * Si el ID del mensaje coincide con el del dispositivo, extrae los parámetros
 * de timestamp y duración, responde con mensajes de estado ("received", "processing")
 * y configura el control de lectura de datos desde la SD.
 * @param topic Tópico en el que se recibió el mensaje.
 * @param payload Contenido del mensaje en bytes.
 * @param length Longitud del mensaje.
 */
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
        Serial.printf("Fecha interpretada: %d \n", controlLectura.fecha);
        controlLectura.timestampInicio = convertirTiempoDelDia(tsUnix);
        Serial.printf("Hora interpretada: %d \n", controlLectura.timestampInicio);
        controlLectura.timestampFin = controlLectura.timestampInicio + duration * 1000;
        Serial.printf("Hora de finalización: %d \n", controlLectura.timestampFin);
        controlLectura.posicionInicial = 0;
        controlLectura.lecturaActiva = true;
        controlLectura.timestampOriginal = (uint32_t)timestamp.toInt();
        controlLectura.error = false; // Reinicia bandera de error
    }
    else
    {
        Serial.println("ID no coincide. Ignorando mensaje.");
    }
}
