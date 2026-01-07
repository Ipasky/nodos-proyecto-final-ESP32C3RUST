# Guión Práctica 4: Lectura de Sensores y MQTT (Arquitectura Secuencial)

## Adaptación de la Práctica 4 a ESP32-C3 sin multitarea explícita

En esta práctica se diseña un nodo IoT capaz de **adquirir datos de sensores** y **publicarlos en un broker MQTT**.

La práctica original proponía el uso de *hilos* (`pthreads`) para leer sensores a diferentes velocidades en paralelo. En esta adaptación **no se utilizará multitarea explícita** (no se crearán tareas adicionales ni hilos “de usuario”). Por tanto, el objetivo es implementar una **arquitectura no bloqueante** dentro del bucle principal (`app_main`), gestionando el tiempo mediante marcas temporales.

La idea es evitar que una lectura lenta (por ejemplo, temperatura) bloquee una lectura rápida (por ejemplo, distancia), simulando concurrencia mediante un esquema de **polling con control de tiempo**.

---

## 4.1. Objetivos

El objetivo final es crear un **nodo de sensores** para un museo que monitorice seguridad (distancia) y condiciones ambientales (temperatura).

Objetivos específicos:

1. **Programación no bloqueante:** gestionar tareas con diferentes frecuencias en un único flujo de ejecución, sin usar esperas largas que detengan el sistema.
2. **Adquisición de señales con ADC:** leer e interpretar la señal analógica de un sensor infrarrojo Sharp (o su equivalente).
3. **Protocolo MQTT:** implementar un cliente MQTT que publique datos de forma periódica en un broker.

> Nota: en esta adaptación los sensores concretos pueden variar según disponibilidad (por ejemplo, GPS y BME/BMP280 en lugar de Sharp), manteniendo la misma lógica de publicación por topics.

---

## 4.2. Requisitos de hardware

* **ESP32-C3 DevKit**
* **Sensor de distancia:** Sharp GP2Y0A41SK0F (analógico) *(o alternativa disponible)*
* **Sensor de temperatura:** sensor externo (recomendado) o valor simulado
* **Conexión Wi-Fi**
* **Broker MQTT** accesible (local o remoto)

---

## 4.3. Desarrollo de la práctica

### 4.3.1. Gestión del tiempo en un bucle único (super-loop)

A diferencia de la versión con hilos (donde cada hilo duerme con su propio `sleep()`), aquí se utilizará un único `while (1)` que ejecuta la lógica de forma repetitiva. Para ejecutar acciones a ritmos distintos se aplica un **patrón de temporización** basado en timestamps.

En ESP-IDF se puede utilizar `esp_timer_get_time()`, que devuelve el tiempo en microsegundos desde el arranque.

**Esquema de funcionamiento:**

```c
int64_t ultimo_tiempo_distancia = 0;
int64_t ultimo_tiempo_temp = 0;

void app_main(void) {
    // Inicialización (WiFi, MQTT, ADC/I2C/UART...)

    while (1) {
        int64_t ahora = esp_timer_get_time();

        // Tarea rápida: Distancia (10 Hz -> cada 100.000 us)
        if (ahora - ultimo_tiempo_distancia >= 100000) {
            leer_sensor_distancia();
            enviar_mqtt_distancia();
            ultimo_tiempo_distancia = ahora;
        }

        // Tarea lenta: Temperatura (0.1 Hz -> cada 10.000.000 us)
        if (ahora - ultimo_tiempo_temp >= 10000000) {
            leer_temperatura();
            enviar_mqtt_temperatura();
            ultimo_tiempo_temp = ahora;
        }

        // Pequeña pausa para evitar saturación de CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

### 4.3.2. Medición de distancia (sensor Sharp)

El sensor Sharp entrega un voltaje analógico que se lee mediante ADC (por ejemplo, con `adc_oneshot`).

* **Reto:** la respuesta del sensor es no lineal.
* **Implementación:** se aplicará una conversión basada en el datasheet (fórmula aproximada) o una tabla de búsqueda (*lookup table*), dentro de la función de lectura.

---

### 4.3.3. Cliente MQTT (ESP-MQTT)

Se utilizará la librería nativa `mqtt_client` de ESP-IDF.

Aunque el programa esté estructurado como un bucle secuencial, el cliente MQTT opera de forma basada en eventos, y permite publicar sin necesidad de bloquear el bucle principal.

Pasos principales:

1. **Configuración del broker:** definir URI, host y puerto (por ejemplo, puerto 1883).
2. **Gestión de eventos:** registrar un manejador (callback) para detectar conexión (`MQTT_EVENT_CONNECTED`) antes de comenzar a publicar.
3. **Publicación por topics:** publicar distancia y temperatura en topics diferenciados.

---

## 4.4. Integración final

El alumno deberá entregar un programa en C con la siguiente estructura:

### 1) Inicialización

* Inicializar NVS (requisito habitual del stack Wi-Fi).
* Conectarse a la red Wi-Fi (esperar a tener IP).
* Inicializar periféricos del sensor (ADC/I2C/UART según el caso).
* Configurar e iniciar el cliente MQTT.

### 2) Bucle principal (super-loop)

El `app_main()` entra en un bucle infinito donde se ejecutan acciones según el tiempo transcurrido:

**Cada 100 ms (10 Hz):**

* Leer el valor del sensor de distancia.
* Convertir a cm (o al formato que corresponda).
* Publicar en el topic:

  * `alumno/museo/distancia`
* *(Opcional)* si `distancia < 10 cm`, imprimir “ALERTA” por consola.

**Cada 10 s (0.1 Hz):**

* Leer temperatura (o generar valor simulado).
* Publicar en el topic:

  * `alumno/museo/temperatura`

---

### 3) Verificación con Mosquitto

Desde un PC se verifica la publicación suscribiéndose a los topics:

```bash
mosquitto_sub -h test.mosquitto.org -t "alumno/museo/#" -v
```

Se debe observar que los mensajes de distancia llegan con mayor frecuencia y, de forma intercalada, aparecen los mensajes de temperatura, demostrando que el bucle único gestiona ambas tareas sin bloqueo.

---

## 4.5. Consideraciones importantes

* **Evitar esperas largas dentro del bucle:** no se deben usar retardos del tipo `vTaskDelay(10000)` dentro del `while(1)`, ya que detendrían también la lectura rápida. El control temporal debe basarse en la comparación de timestamps (`if (ahora - antes >= periodo)`).
* **Simplicidad y seguridad:** este enfoque evita condiciones de carrera típicas de la multitarea (varios hilos accediendo a variables compartidas), ya que la lógica principal se ejecuta de forma ordenada.

---

## 4.6. Nota sobre la arquitectura de “varios nodos” en ESP32

En Raspberry Pi era posible ejecutar **varios programas** (procesos) a la vez: uno para el nodo sensor, otro para alarma y otro para climatización.

En ESP32 esto cambia: la placa normalmente ejecuta **un único firmware** cargado en flash. Por tanto, para reproducir la arquitectura de varios nodos existen dos opciones:

1. **Varias placas (recomendado si se dispone de ellas):**

   * Placa 1: firmware “sensor”
   * Placa 2: firmware “alarma”
   * Placa 3: firmware “clima”
     Cada placa se conecta al broker y realiza su rol, emulando de forma fiel el esquema original.

2. **Una sola placa ejecutando los 3 roles dentro del mismo firmware:**
   En este caso, el programa integra la lógica de sensor + alarma + clima en una única aplicación. La separación no es por procesos, sino por módulos y por temporización dentro del super-loop.