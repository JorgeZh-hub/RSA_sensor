# Proyecto ESP32: Registro de Señales Acelerométricas

## Descripción
Esta guía describe la instalación, programación y uso de un sensor implementado en un microcontrolador ESP32 para el registro continuo de señales acelerométricas utilizando diversos periféricos.

## Periféricos
El sistema sensor está compuesto por los siguientes cuatro módulos principales:

### 1. ADXL355
Un acelerómetro MEMS de alta precisión capaz de medir la aceleración en los tres ejes (X, Y, Z). Este dispositivo ofrece:
- Diversas tasas de muestreo.
- Estabilidad a largo plazo.
- Baja deriva.

Es ideal para aplicaciones que requieren alta precisión y confiabilidad en el tiempo.

### 2. GPS FGPMMOPA6H
Un módulo GPS que proporciona:
- Datos de ubicación.
- Velocidad.
- Tiempo mediante señales satelitales.

Es fundamental para sincronizar datos geoespaciales y temporales.

### 3. Módulo RTC (Real Time Clock) DS3231
Un reloj en tiempo real de alta precisión que incluye:
- Oscilador de cristal compensado por temperatura.
- Seguimiento preciso del tiempo, incluso sin alimentación externa.

### 4. Módulo SD
Un módulo de almacenamiento que utiliza la interfaz SPI para guardar datos en una tarjeta SD. Permite el registro eficiente de grandes volúmenes de datos.

## Programación
El desarrollo del sistema sensor se realizó utilizando:
- **Framework:** Arduino.
- **Entorno de desarrollo:** PlatformIO.

PlatformIO es un entorno de desarrollo integrado (IDE) multiplataforma que ofrece herramientas avanzadas para:
- Gestíon de proyectos.
- Manejo de bibliotecas.
- Compilación.

Estas herramientas optimizan el flujo de trabajo en dispositivos embebidos.

## Implementación Física
La implementación inicial del dispositivo se llevó a cabo sobre una protoboard, lo que permitió:
- Montaje modular de componentes.
- Pruebas flexibles para asegurar el correcto funcionamiento.

## Instalación y Uso
1. **Preparar el entorno de desarrollo:**
   - Instalar PlatformIO en su editor de código preferido (por ejemplo, Visual Studio Code).
   - Descargar las bibliotecas necesarias para los periféricos (ADXL355, GPS, RTC, y SD).

2. **Configurar el hardware:**
   - Conectar los módulos al ESP32 siguiendo el esquema de pines especificado en la documentación.

3. **Cargar el firmware:**
   - Compilar y cargar el código al ESP32 utilizando PlatformIO.

4. **Realizar pruebas:**
   - Verificar que todos los módulos estén funcionando correctamente mediante las salidas en el monitor serie.

5. **Iniciar el registro:**
   - Asegurar que la tarjeta SD esté insertada y comenzar el registro de datos.

## Licencia
Este proyecto es de libre acceso

## Contribuciones
Las contribuciones son bienvenidas. Por favor, abre un issue o envía un pull request para sugerencias o mejoras.

