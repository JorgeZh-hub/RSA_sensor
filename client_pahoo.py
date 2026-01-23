import paho.mqtt.client as mqtt
import struct
import json
import time
import matplotlib.pyplot as plt
from datetime import timedelta
import matplotlib.dates as mdates
from datetime import datetime, timedelta

# --- Configuración del broker ---
# --- Configuración MQTT ---
MQTT_BROKER = "192.168.0.101"
MQTT_PORT = 1883
MQTT_USER = ""
MQTT_PASS = ""

TOPIC_SUBSCRIBE = "events/ESP32_001/data"
TOPIC_PUBLISH = "events/ESP32_001/request"

# --- Variables globales ---
tiempos = []
t_strs = []
x_vals = []
y_vals = []
z_vals = []

fin_recibido = False  # bandera de finalización


# --- Conexión al broker ---
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Conectado al broker")
        client.subscribe(TOPIC_SUBSCRIBE)
    else:
        print(f"❌ Fallo al conectar. Código: {rc}")


# --- Procesar mensaje recibido ---
def on_message(client, userdata, msg):
    global tiempos, x_vals, y_vals, z_vals, fin_recibido, t_strs

    try:
        # Ver si es un mensaje JSON con "status":"finished"
        payload_str = msg.payload.decode("utf-8")
        data = json.loads(payload_str)
        if isinstance(data, dict) and data.get("status") == "finished":
            print("✅ Transmisión terminada (status: finished)")
            fin_recibido = True
            return
    except (UnicodeDecodeError, json.JSONDecodeError):
        pass  # No es un JSON válido, asumir binario

    # Si no es JSON, intentar desempaquetar binario
    if len(msg.payload) != 16:
        print(f"⚠️ Tamaño inesperado de mensaje ({len(msg.payload)} bytes)")
        return

    try:
        timestamp, x, y, z = struct.unpack("<iiii", msg.payload)
        tiempo_legible = str(timedelta(milliseconds=timestamp))[:-3]

        print(f"📦 {tiempo_legible} -> x: {x}, y: {y}, z: {z}")

        tiempos.append(timestamp)
        t_strs.append(tiempo_legible)
        x_vals.append(x * 0.00374)
        y_vals.append(y * 0.00374)
        z_vals.append(z * 0.00374)

    except struct.error as e:
        print(f"❌ Error al desempaquetar: {e}")


# --- Crear cliente y conectar ---
client = mqtt.Client()
if MQTT_USER:
    client.username_pw_set(MQTT_USER, MQTT_PASS)

client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
client.loop_start()

# --- Solicitar datos ---
try:
    print("\n--- Ingresar datos para solicitar ---")
    id_ = input("ID: ").strip()
    timestamp = int(input("Timestamp inicial (UNIX): ").strip())
    duration = int(input("Duración (segundos): ").strip())

    payload = {"id": id_, "timestamp": timestamp, "duration": duration}
    client.publish(TOPIC_PUBLISH, json.dumps(payload))
    print(f"📤 Solicitud enviada: {payload}")
    print("⌛ Esperando datos...")

    # Esperar hasta recibir "status":"finished"
    tiempo_inicio = time.time()
    timeout = duration + 20  # margen de seguridad
    while not fin_recibido and time.time() - tiempo_inicio < timeout:
        time.sleep(0.1)

    client.loop_stop()
    client.disconnect()

    # Convertir timestamps (milisegundos desde medianoche) a datetime.datetime del día actual
    tiempos_dt = [
        datetime.combine(datetime.today(), datetime.min.time())
        + timedelta(milliseconds=ts)
        for ts in tiempos
    ]

    # --- Graficar ---
    if tiempos:
        plt.figure(figsize=(10, 6))
        plt.plot(tiempos_dt, x_vals, label="X (cm/s²)")
        plt.plot(tiempos_dt, y_vals, label="Y (cm/s²)")
        plt.plot(tiempos_dt, z_vals, label="Z (cm/s²)")

        plt.xlabel("Hora")
        plt.ylabel("Aceleración (cm/s²)")
        plt.title("Datos de aceleración recibidos")

        # Formatear el eje X con horas:minutos:segundos
        plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
        plt.gca().xaxis.set_major_locator(mdates.AutoDateLocator())

        plt.xticks(rotation=45)
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()
    else:
        print("⚠️ No se recibieron datos.")

except KeyboardInterrupt:
    print("🛑 Interrumpido por el usuario.")
    client.loop_stop()
    client.disconnect()
