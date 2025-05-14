import paho.mqtt.client as mqtt
import json
import time

# Configuración del broker
MQTT_BROKER = "174.138.41.251"  # Reemplaza con IP o dominio
MQTT_PORT = 1883
MQTT_USER = "rsa"
MQTT_PASS = "RSAiotace2023"

# Tópicos
TOPIC_SUBSCRIBE = "/events/ESP32_001/data"
TOPIC_PUBLISH = "events/ESP32_001/request"


# Callback de conexión
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado al broker")
        client.subscribe(TOPIC_SUBSCRIBE)
    else:
        print(f"Fallo al conectar. Código de error: {rc}")


# Callback de mensaje recibido
def on_message(client, userdata, msg):
    print(f"Mensaje recibido en {msg.topic}: {msg.payload.decode()}")


# Crear cliente
client = mqtt.Client()

if MQTT_USER:
    client.username_pw_set(MQTT_USER, MQTT_PASS)

client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
client.loop_start()

# Esperar conexión
time.sleep(2)

# Leer datos desde consola
try:
    while True:
        print("\n--- Ingresar datos para publicar ---")
        id_ = input("ID: ").strip()
        timestamp = input("Timestamp (UNIX): ").strip()
        duration = input("Duración (segundos): ").strip()

        try:
            payload = {"id": id_, "timestamp": timestamp, "duration": int(duration)}
            client.publish(TOPIC_PUBLISH, json.dumps(payload))
            print(f"Publicado: {payload}")
        except ValueError:
            print("Error: duración debe ser un número entero.")

        time.sleep(1)

except KeyboardInterrupt:
    print("\nDesconectando...")
    client.loop_stop()
    client.disconnect()
