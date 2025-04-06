# -*- coding: utf-8 -*-
"""
Created on Thu Jan  2 21:04:10 2025

@author: geova
"""
import struct
import matplotlib.pyplot as plt
from collections import defaultdict

# Función para leer y decodificar datos binarios
def read_bin_file(filename):
    measurements = []
    with open(filename, "rb") as f:
        while True:
            # Cada medición tiene 20 bytes
            chunk = f.read(20)
            if len(chunk) < 20:
                break  # Salir si no hay suficientes datos
            
            # Decodificar los datos binarios en formato little-endian
            date, milliseconds, x, y, z = struct.unpack("<I I i i i", chunk)
            
            measurements.append({
                "date": date,
                "milliseconds": milliseconds,
                "x": x,
                "y": y,
                "z": z
            })
    return measurements

# Función para calcular datos por segundo
def calculate_data_per_second(measurements):
    data_count = defaultdict(int)
    for m in measurements:
        # Convertir milisegundos al segundo correspondiente
        second = m["milliseconds"] // 1000
        data_count[second] += 1
    return data_count

# Función para graficar las aceleraciones
def plot_accelerations(measurements):
    times = [m["milliseconds"] / 1000 for m in measurements]  # Convertir a segundos
    x_values = [m["x"]*0.00374 for m in measurements]
    y_values = [m["y"]*0.00374 for m in measurements]
    z_values = [m["z"]*0.00374 for m in measurements]
    
    
    plt.figure(figsize=(12, 6))
    plt.plot(times, x_values, label="X-axis", color="red")
    plt.plot(times, y_values, label="Y-axis", color="green")
    plt.plot(times, z_values, label="Z-axis", color="blue")
    plt.xlabel("Time (seconds)")
    plt.ylabel("Acceleration (cm/s2)")
    plt.title("Accelerometer Data")
    plt.legend()
    plt.grid()
    plt.show()

# Procesamiento principal
filename = "E:\data0.bin" 
measurements = read_bin_file(filename)

# Calcular datos por segundo
data_per_second = calculate_data_per_second(measurements)
print("Datos por segundo:", dict(data_per_second))

# Graficar aceleraciones
plot_accelerations(measurements)

