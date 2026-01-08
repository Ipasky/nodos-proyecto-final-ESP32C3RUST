# Guión Práctica 4: Sistema IoT con MQTT y Visualización Web

## 1. Objetivo de la práctica

Diseñar e implementar un sistema IoT capaz de:

* **Adquirir datos** de dos fuentes distintas:

  * **GPS (NEO-7M)** mediante **UART** (tramas NMEA).
  * **Temperatura y presión (BMP/BME280)** mediante **I2C**.
* **Publicar** dichos datos por **MQTT** a un broker (en este caso **Mosquitto en local**).
* **Visualizar** los datos en tiempo real mediante un **servidor Node.js** que actúa como **suscriptor MQTT** y ofrece una **interfaz web**.

---

## 2. Diferencias respecto a la práctica original

En la práctica original (Raspberry Pi + Linux) se ejecutaban **varios programas independientes** (sensores, alarma, clima…) y se usaban **hilos POSIX (`pthreads`)** para paralelizar lecturas.

En **ESP32-C3** esto cambia por dos motivos:

1. **No existe multiproceso** como en Linux (no podemos lanzar “3 programas” separados como procesos).
2. La ESP32-C3 es **single-core**, así que **no hay paralelismo real** (no ejecuta dos cosas “a la vez” físicamente). 

### Solución aplicada: FreeRTOS (concurrencia con tareas)

Para suplirlo, el firmware se estructura con **FreeRTOS Tasks**. Aunque haya un solo núcleo, el **scheduler** reparte el tiempo de CPU y alterna rápidamente entre tareas (**concurrencia por time-slicing**). 

---

## 3. Arquitectura final del sistema

**Nodo 1 (ESP32-C3) – Nodo Sensor / Edge Node**

* Lee GPS (UART) + BMP/BME280 (I2C)
* Publica por MQTT en `sensor/data`

**Nodo 2 (PC – Mosquitto) – Broker MQTT**

* Broker local en `localhost:1883`

**Nodo 3 (PC – Node.js) – Suscriptor + Dashboard Web**

* Se suscribe a `sensor/data`
* Reenvía datos al navegador (WebSockets) y los muestra en una interfaz web

---

## 4. Hardware y conexiones

### 4.1. ESP32-C3 + GPS NEO-7M (UART)

* GPS **TX → RX** de la ESP32 (pin RX configurado en el código)
* GPS **RX → TX** de la ESP32 (solo necesario si vas a enviar comandos al GPS)
* **GND común** siempre obligatorio
* Alimentación: según tu módulo (muchos NEO-7M aceptan 5V en “VCC” si llevan regulador, pero la lógica UART suele ser 3.3V; si dudas, usa 3.3V o mide la salida TX del módulo).

### 4.2. ESP32-C3 + BMP/BME280 (I2C)

* SDA ↔ SDA
* SCL ↔ SCL
* VCC (normalmente 3.3V en módulos para MCU)
* GND común

---

## 5. Firmware en ESP-IDF: estructura basada en FreeRTOS

El firmware se implementa como **tres tareas principales**, además de las tareas internas del stack de WiFi/MQTT (que también corren sobre FreeRTOS).

### Tarea A — Lectura ambiental (BMP/BME280 por I2C)

* **Frecuencia:** baja (aprox. 1 Hz)
* **Función:** leer temperatura y presión, aplicar compensación/calibración del sensor y actualizar variables globales.

### Tarea B — Lectura GPS (UART, orientada a eventos)

* **Frecuencia:** alta (depende del flujo de datos NMEA)
* **Función:** leer bytes del UART y reconstruir líneas NMEA (por ejemplo `$GPRMC`, `$GPGLL`, etc.).
* Guarda la última trama relevante para poder publicarla.

### Tarea C — Publicación MQTT

* **Frecuencia:** media (por ejemplo cada 2–5 s, según configuración)
* **Función:**

  1. Verificar que MQTT está conectado.
  2. Tomar el último valor de temperatura, presión y GPS.
  3. Construir un mensaje **JSON**.
  4. Publicar en el topic `sensor/data`.

Este diseño coincide con el enfoque de “tareas separadas” que se describe en tu guión, sustituyendo los “programas independientes” de Linux por **tareas de FreeRTOS dentro de un único firmware**.

---

## 6. Sincronización y datos compartidos

Como varias tareas comparten información (por ejemplo, la tarea GPS escribe una trama y la tarea MQTT la lee), es necesario proteger los datos para evitar lecturas corruptas.

Opciones típicas:

* **Mutex/Semáforo**
* **Colas (Queues)**
* **Secciones críticas (`portENTER_CRITICAL`)** si el intercambio es corto y simple

En esta práctica se utiliza un mecanismo de sincronización simple para garantizar coherencia cuando una tarea actualiza datos que otra tarea va a publicar. 

---

## 7. Formato del mensaje MQTT

Se utiliza un JSON para simplificar la integración con Node.js:

```json
{"temp": 24.5, "press": 1013.2, "gps": "$GNRMC,..."}
```

* Si el GPS **no ha actualizado** (o no hay FIX), se publica un placeholder (por ejemplo `"-"` o una trama con estado `V`), para que la web pueda mostrar “sin señal” de forma clara. 

---

## 8. Nodo Servidor (Node.js): suscriptor + dashboard

En el PC se implementa una aplicación **Node.js** con dos roles:

1. **Suscriptor MQTT (backend):**

   * Se conecta al broker local (`mqtt://localhost:1883`)
   * Se suscribe a `sensor/data`
   * Cada mensaje recibido se procesa y se reenvía al frontend

2. **Servidor web (frontend):**

   * Sirve una página web (por ejemplo con Express)
   * Envía datos en tiempo real con WebSockets (por ejemplo Socket.io)
   * Muestra:

     * Tarjetas/valores: temperatura y presión
     * (Opcional) Mapa: si se parsean coordenadas del GPS

---

## 9. Verificación y puesta en marcha

### 9.1. Broker Mosquitto (Windows, local)

* Asegurar que el servicio Mosquitto está activo.
* Probar suscripción en una terminal (si `mosquitto_sub` no aparece, suele ser un problema de PATH):

```bat
mosquitto_sub -h localhost -t sensor/data -v
```

### 9.2. ESP32

Flashear y monitorizar:

```bash
idf.py build flash monitor
```

Comprobar en consola:

* Conexión WiFi
* Conexión MQTT
* Publicaciones periódicas en `sensor/data`

### 9.3. Node.js

Ejecutar el servidor y abrir el navegador:

```txt
http://localhost:3000
```

Verificar que:

* Llegan JSONs al servidor
* La web se actualiza sin recargar

---

## 10. Conclusión

Esta adaptación transforma una arquitectura “tipo Linux” (multiproceso + hilos POSIX) en una arquitectura IoT realista basada en **un único firmware** sobre ESP32-C3, donde la concurrencia se logra con **FreeRTOS Tasks** y un **scheduler** que reparte el tiempo de CPU. De esta forma, el sistema mantiene lecturas de sensores y comunicación MQTT sin bloquear el funcionamiento, pese a trabajar en un microcontrolador **single-core**.