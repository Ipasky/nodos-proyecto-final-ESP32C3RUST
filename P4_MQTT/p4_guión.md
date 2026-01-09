# Guión Práctica 4: Sistema IoT Distribuido: Sensorización, MQTT y Visualización

## Adaptación de la Práctica 4 a la plataforma ESP32-C3

El objetivo de esta práctica es el diseño e implementación de un sistema **IoT (Internet of Things)** completo, abordando la cadena de valor del dato desde su adquisición física hasta su visualización remota.

La práctica original estaba diseñada sobre una Raspberry Pi con sistema operativo Linux, donde se ejecutaban múltiples procesos independientes (sensores, alarma, climatización) comunicados entre sí. Para la paralelización de tareas se utilizaba la librería POSIX Threads (`pthreads`).

En esta adaptación para la **ESP32-C3**, migramos la arquitectura a un entorno de microcontrolador industrial. Esto implica desafíos arquitectónicos importantes:

1. **Restricción de Hardware:** Pasamos de un procesador multi-núcleo (ARM Cortex-A) a un microcontrolador **Single-Core** (RISC-V).
2. **Gestión de Concurrencia:** Sustituimos el paralelismo real y los hilos del SO por un **Sistema Operativo en Tiempo Real (FreeRTOS)** que gestiona la concurrencia mediante *Time Slicing*.
3. **Visualización:** Sustituimos los actuadores locales por un **Dashboard Web** remoto basado en Node.js.

## Cambios en la plataforma hardware

La ESP32-C3 integra controladores hardware dedicados para los protocolos de comunicación necesarios, lo que permite liberar a la CPU de la gestión de señales bit a bit (*bit-banging*).

La práctica se centra en la integración simultánea de dos buses de comunicación distintos:

* **Protocolo UART (Asíncrono):** Para la recepción de tramas NMEA del **GPS NEO-7M**.
* **Protocolo I2C (Síncrono):** Para la lectura de registros del sensor ambiental **BMP280/BME280**.

### Esquema de Conexiones

| Dispositivo | Interfaz | Pines ESP32-C3 | Descripción |
| --- | --- | --- | --- |
| **GPS NEO-7M** | UART1 | TX GPS  GPIO 20 (RX) | Recepción de flujo de datos continuo |
|  |  | RX GPS  GPIO 21 (TX) | (Opcional) Envío de comandos de configuración |
| **BMP280** | I2C | SDA  GPIO 8 | Línea de datos bidireccional |
|  |  | SCL  GPIO 10 | Línea de reloj (Master: ESP32) |

---

## Configuración a bajo nivel de los periféricos

Al igual que en las prácticas anteriores, aunque utilizaremos drivers de alto nivel, es fundamental comprender qué ocurre dentro del silicio. La ESP32-C3 gestiona estos protocolos mediante registros mapeados en memoria.

### Registros principales del periférico UART (GPS)

La comunicación con el GPS es asíncrona y continua. El hardware debe ser capaz de almacenar los datos que llegan mientras la CPU está ocupada en otras tareas.

#### UART_FIFO_REG

Es el registro de acceso a la memoria. La ESP32-C3 dispone de un **buffer circular hardware (FIFO)** de 128 bytes.

* Cuando el GPS envía un byte, este entra automáticamente en la FIFO de recepción (`RX_FIFO`).
* La CPU lee de este registro para vaciar la FIFO. Si la CPU no lee lo suficientemente rápido y la FIFO se llena, los nuevos datos se perderán (evento *FIFO Overflow*).

#### UART_INT_ENA_REG

Habilita las interrupciones. Para un manejo eficiente del GPS, es crítico habilitar la interrupción `RXFIFO_FULL_INT` (cuando el buffer tiene cierta cantidad de datos) o `RXFIFO_TOUT_INT` (cuando han llegado datos pero la línea se ha quedado en silencio, indicando fin de trama), para que la CPU solo atienda al GPS cuando sea necesario.

### Registros principales del periférico I2C (Sensores)

#### I2C_MASTER_TR_REG

Al contrario que la UART, el I2C es síncrono. Este registro gestiona la cola de comandos que el maestro (ESP32) enviará al bus. Aquí se cargan secuencialmente: la dirección del esclavo (0x76 para BMP280), el bit de lectura/escritura y los datos.

#### I2C_SDA_SAMPLE_REG

Controla el momento exacto en el que el hardware muestrea la línea de datos SDA respecto al flanco del reloj SCL. Una desincronización en este registro a altas velocidades (100kHz o 400kHz) provocaría errores de comunicación ACK/NACK.

---

## Cambios en el entorno software: De Hilos a Tareas

La diferencia conceptual más profunda de esta práctica reside en el modelo de ejecución.

### Justificación: ¿Por qué no usamos `pthreads`?

En la práctica original (Linux), `pthread_create` solicitaba al Núcleo del Sistema Operativo que crease un nuevo hilo de ejecución. Si el procesador tenía varios núcleos, estos hilos podían ejecutarse **físicamente a la vez** (Paralelismo).

En la ESP32-C3, solo tenemos un núcleo. Si intentásemos ejecutar dos bucles `while(1)` infinitos, el primero bloquearía al segundo para siempre.

### Solución: El Planificador (Scheduler) de FreeRTOS

Para suplir esto, utilizamos **FreeRTOS**. El *Scheduler* es un componente software que detiene e inicia tareas tan rápido que da la sensación de simultaneidad (**Concurrencia**).

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

> **Importante:** A diferencia de la programación secuencial clásica (Arduino), aquí **nunca** se debe usar un `delay()` bloqueante que detenga la CPU. Se deben usar las funciones del sistema operativo (`vTaskDelay`) que indican al planificador: "Cede mi turno de CPU a otra tarea durante X milisegundos".

---

## Desarrollo de la Práctica

### 1. Integración de Drivers (UART e I2C)

Se deben inicializar los drivers `uart` y `i2c_master` de ESP-IDF. Estos drivers abstraen la complejidad de los registros mencionados anteriormente (`UART_FIFO_REG`, etc.) y gestionan internamente las interrupciones (ISRs) para llenar buffers de software intermedios.

### 2. Implementación del Cliente MQTT

Se utilizará la librería `esp-mqtt`. El protocolo MQTT se basa en el modelo **Publicación/Suscripción**, ideal para dispositivos IoT con ancho de banda limitado:

* **Broker:** El servidor central (Mosquitto en vuestro PC).
* **Topic:** La "frecuencia" o canal donde publicamos. Usaremos `sensor/data`.
* **Payload:** El mensaje. Para facilitar el procesado, se usará formato **JSON**.

```json
{
  "temp": 24.5,
  "press": 1013.2,
  "gps": "$GNRMC,123519,A,4807.038,N..."
}

```

### 3. Desarrollo del Nodo Servidor (node.js)

Para cerrar el ciclo, no usaremos actuadores físicos, sino digitales. Se implementará un servidor en **Node.js** que cumplirá dos funciones simultáneas:

1. **Suscriptor MQTT (backend):**

   * Se conecta al broker local (`mqtt://localhost:1883`)
   * Se suscribe a `sensor/data`
   * Cada mensaje recibido se procesa y se reenvía al frontend

2. **Servidor web (frontend):**

   * Sirve una página web
   * Envía datos en tiempo real con WebSockets (por ejemplo `socket.io`)
   * Muestra:

     * Tarjetas/valores: temperatura y presión
     * Mapa: si se parsean coordenadas del GPS

---

## Análisis de los Resultados

### Verificación del Sistema

Para validar el correcto funcionamiento de la arquitectura distribuida, se deben seguir los siguientes pasos:

1. **Arranque del Broker:** Verificar que Mosquitto está corriendo en el PC (`localhost:1883`).
2. **Monitorización Serie:** Al arrancar la ESP32, el log debe mostrar la secuencia:
* `I2C Init OK` -> `UART Init OK`.
* `WiFi Connected` -> `IP Assigned`.
* `MQTT Connected`.


3. **Recepción de Datos:**
* En la consola del PC (Node.js), deben aparecer los objetos JSON llegando periódicamente.
* Si el GPS está en interiores, el campo GPS puede estar vacío o contener una trama con estado "V" (Void), pero la temperatura debe ser correcta.


4. **Dashboard Web:** Accediendo a `http://localhost:3000`, los widgets de temperatura deben actualizarse cada vez que el ESP32 publica, y el mapa debe centrarse en las coordenadas recibidas.

### Conclusión

Esta práctica demuestra cómo transformar una arquitectura basada en microprocesadores potentes (Raspberry Pi) a una arquitectura de microcontroladores eficientes (ESP32). La pérdida de potencia de cálculo (Single-Core) se compensa mediante una gestión inteligente de los recursos (FreeRTOS) y el uso de periféricos hardware dedicados (DMA, FIFO), permitiendo crear nodos IoT industriales robustos y de bajo consumo.