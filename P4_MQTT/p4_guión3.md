# Guión Práctica 4: Sistema IoT Distribuido con MQTT y Visualización Web

## Adaptación de la Práctica 4 a la plataforma ESP32-C3

El objetivo de esta práctica es el diseño e implementación de un sistema distribuido para la monitorización remota de variables ambientales y posicionamiento.

La práctica original planteaba un escenario de "Museo" con tres programas independientes ejecutándose en una Raspberry Pi (Linux): un nodo de sensores, un nodo actuador (alarma) y un nodo de climatización. Estos programas se comunicaban entre sí mediante un *Broker* MQTT local, aprovechando la capacidad del sistema operativo Linux para gestionar múltiples procesos e hilos (`pthreads`) en paralelo.

En esta adaptación para el microcontrolador **ESP32-C3**, migramos la arquitectura a un entorno de **Internet de las Cosas (IoT)** más realista y moderno. En lugar de simular nodos en una misma máquina, tendremos un **Nodo de Borde (Edge Node)** físico (la ESP32) enviando datos a través de la red WiFi a un **Servidor de Visualización** remoto (PC con Node.js).

## Cambios en la Arquitectura y el Hardware

### 1. Del Paralelismo a la Concurrencia (El reto del Single-Core)

Una diferencia fundamental entre la Raspberry Pi y la ESP32-C3 es la arquitectura del procesador.

* **Original (Raspberry Pi):** Dispone de un procesador Multi-Núcleo con un Sistema Operativo completo (Linux). Esto permite **Paralelismo Real** (ejecutar varias instrucciones al mismo tiempo) y **Multiproceso** (programas separados con memoria aislada).
* **Adaptación (ESP32-C3):** Cuenta con una arquitectura **RISC-V de Núcleo Único (Single-Core)**. Físicamente, **no es posible** ejecutar dos instrucciones simultáneamente.

**Solución Implementada: FreeRTOS**
Para suplir la falta de paralelismo real y cumplir con el requisito de leer múltiples sensores a diferentes frecuencias, utilizamos un **Sistema Operativo en Tiempo Real (RTOS)**, concretamente **FreeRTOS**, integrado en el framework ESP-IDF.

En lugar de "Hilos" o "Procesos", utilizamos **Tareas (Tasks)**. El planificador (*Scheduler*) de FreeRTOS divide el tiempo de CPU en fragmentos minúsculos (*Time Slicing*), alternando rápidamente entre la tarea del GPS, la del sensor BMP280 y la de comunicación MQTT. Esto crea una ilusión de paralelismo (**Concurrencia**) que permite al sistema reaccionar a múltiples eventos "a la vez", aunque técnicamente se procesen secuencialmente.

### 2. Hardware de Sensores

Sustituimos los sensores originales (Infrarrojos Sharp y Temperatura analógica) por sensores digitales que requieren protocolos de comunicación estándar, aumentando la complejidad de la integración:

* **Posicionamiento:** Módulo GPS NEO-7M (Protocolo UART / NMEA).
* **Ambiental:** Sensor BMP280/BME280 (Protocolo I2C) para Temperatura y Presión.

### 3. Visualización Web (Dashboard)

Sustituimos los "Nodos Actuadores" (programas de consola que encendían LEDs) por un **Servidor Web moderno**. Se implementa un backend en **Node.js** que actúa como suscriptor MQTT y sirve una interfaz gráfica en HTML/JS, permitiendo visualizar la posición en un mapa y los datos en tiempo real.

---

## 4.1. Objetivos

1. **Gestión de Tareas en RTOS:** Implementar un sistema multitarea en un microcontrolador *single-core* usando FreeRTOS, gestionando las prioridades para que la comunicación WiFi/MQTT no bloquee la lectura de sensores.
2. **Protocolos de Comunicación:** Integrar simultáneamente un sensor por **I2C** (Síncrono) y otro por **UART** (Asíncrono).
3. **IoT & MQTT:** Implementar un ciclo completo de telemetría: Sensor  Broker  Cliente Web.
4. **Visualización:** Representar datos geoespaciales y ambientales en una interfaz de usuario.

---

## 4.2. Desarrollo de la Práctica: El Nodo Sensor (ESP32)

El *firmware* de la ESP32 debe estructurarse en tres tareas principales independientes gestionadas por el *scheduler*:

### Tarea 1: Adquisición Ambiental (BMP280) [I2C]

* **Frecuencia:** Baja (~1 Hz).
* **Función:** Debe inicializar el bus I2C y configurar el sensor BMP280. En cada ciclo, lee los registros de temperatura y presión, aplica las fórmulas de compensación (calibración de fábrica) y actualiza las variables globales.
* **Recurso Crítico:** El bus I2C.

### Tarea 2: Adquisición de Posicionamiento (GPS) [UART]

* **Frecuencia:** Alta / Orientada a Eventos.
* **Función:** Debe leer continuamente el buffer del puerto UART. El módulo GPS envía tramas de texto ASCII (NMEA 0183) constantemente. La tarea debe detectar el carácter de fin de línea `\n`, capturar la trama completa (ej. `$GNRMC...`) y guardarla para su envío.
* **Sincronización:** Dado que esta tarea escribe en un buffer compartido que otra tarea leerá, se recomienda el uso de **Semáforos (Mutex)** o secciones críticas (`portENTER_CRITICAL`) para evitar corrupción de memoria.

### Tarea 3: Comunicación y Publicación (MQTT)

* **Frecuencia:** Media (ej. cada 2 segundos).
* **Función:** Actúa como el puente con el exterior.
1. Verifica si hay conexión WiFi y MQTT.
2. Toma los últimos datos válidos de las variables globales (Temperatura, Presión, Trama GPS).
3. Formatea los datos en un estructura **JSON** (JavaScript Object Notation). Ejemplo:
```json
{"temp": 24.5, "press": 1013.2, "gps": "$GNRMC,..."}

```


4. Publica el mensaje en el tópico `sensor/data` del Broker.



---

## 4.3. Desarrollo de la Práctica: El Nodo Servidor (Node.js)

Para cerrar el sistema, se desarrolla una aplicación en el lado del servidor (PC) utilizando **Node.js**. Este componente cumple dos funciones:

1. **Suscriptor MQTT (Backend):** Utiliza la librería `mqtt.js` para conectarse al mismo Broker que la ESP32 y suscribirse al tópico `sensor/data`. Cada vez que llega un mensaje, lo procesa y lo reenvía.
2. **Servidor Web (Frontend):** Utiliza `Express` para servir una página web y `Socket.io` para enviar los datos al navegador en tiempo real (WebSockets).

### Interfaz Gráfica

La página web (`index.html`) debe incluir:

* **Widgets de Estado:** Tarjetas visuales para Temperatura y Presión.
* **Mapa Interactivo:** Integración de la librería **Leaflet.js** . El código JavaScript del navegador debe parsear la trama NMEA cruda del GPS, convertir las coordenadas de formato "Grados-Minutos" a "Grados Decimales" y actualizar un marcador en el mapa.
* **Lógica de Fallback:** Si el GPS no tiene cobertura (interior de edificios), el sistema debe indicarlo visualmente o posicionar el mapa en una ubicación predefinida (ej. Facultad) para efectos de demostración.

---

## 4.4. Integración y Verificación

Para dar por finalizada la práctica, el alumno debe demostrar el funcionamiento conjunto de los sistemas:

1. **Arranque:** Al conectar la ESP32, se debe observar por el puerto serie la inicialización de los drivers (UART, I2C) y la conexión WiFi.
2. **Conexión:** El servidor Node.js debe mostrar en su consola "Cliente Conectado" y comenzar a recibir los JSONs.
3. **Visualización:**
* Abrir el navegador en `http://localhost:3000`.
* Verificar que los valores de temperatura y presión se actualizan automáticamente sin recargar la página.
* Verificar que el mapa muestra la ubicación o el estado de "Buscando satélites".