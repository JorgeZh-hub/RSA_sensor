import paho.mqtt.client as mqtt
import struct

def on_message(client, userdata, msg):
    try:
        # Deserializa los 20 bytes: 2 uint32_t (I) + 3 int32_t (i)
        timestamp, x, y, z = struct.unpack("<Iiii", msg.payload)
        print(f"Timestamp: {timestamp}, X: {x}, Y: {y}, Z: {z}")
    except Exception as e:
        print(f"Error al decodificar: {e}")
        print(f"Payload HEX: {msg.payload.hex()}")  # Debug: muestra los bytes en hexadecimal

client = mqtt.Client()
client.connect("test.mosquitto.org", 1883)
client.subscribe("sensor/datos_bin")
client.on_message = on_message
client.loop_forever()