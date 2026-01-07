# Guión Práctica 4: Lectura de Sensores y MQTT (Arquitectura Secuencial)

## Adaptación de la Práctica 4 a ESP32-C3 sin Multitarea explícita

En esta práctica diseñaremos un nodo IoT capaz de adquirir datos y enviarlos a internet.

La práctica original proponía el uso de *hilos* (`pthreads`) para leer sensores a diferentes velocidades en paralelo. Dado que en esta adaptación **no utilizaremos multitarea explícita** (no crearemos tareas ni hilos adicionales), el reto será implementar una **arquitectura de programación no bloqueante** dentro del bucle principal (`app_main`).

Deberemos gestionar el tiempo manualmente para lograr que una lectura lenta no detenga a la rápida, simulando un comportamiento concurrente mediante el uso de cronómetros internos (Polling con control de tiempo).

---

## 4.1. Objetivos

El objetivo final es crear un **Nodo de Sensores** para un museo que monitorice la seguridad (distancia) y el clima (temperatura).

Los objetivos específicos son:

1. **Programación No Bloqueante:** Aprender a gestionar múltiples tareas con diferentes frecuencias (10 Hz y 0.1 Hz) en un único hilo de ejecución sin usar esperas largas (`delay`).
2. **Sensores Analógicos:** Interpretar la señal de un sensor infrarrojo Sharp.
3. **Protocolo MQTT:** Implementar un cliente que publique datos en un *Broker* de forma asíncrona.

---

## 4.2. Requisitos Hardware

* **ESP32-C3 DevKit**.
* **Sensor de Distancia:** Infrarrojo SHARP GP2Y0A41SK0F (analógico).
* **Sensor de Temperatura:** (Se puede simular o usar el interno del chip).
* **Conexión WiFi.**

---

## 4.3. Desarrollo de la práctica

### 4.3.1. Gestión del Tiempo en un Bucle Único (Single-Loop)

A diferencia de la versión con hilos donde cada uno tiene su propio `sleep()`, aquí tendremos un único `while(1)` que se ejecuta miles de veces por segundo. Para ejecutar acciones a ritmos diferentes, usaremos el "patrón del cronómetro" (similar al *Blink Without Delay* de Arduino).

Usaremos la función de ESP-IDF `esp_timer_get_time()`, que devuelve el tiempo del sistema en microsegundos ().

**Lógica del programa:**

```c
int64_t ultimo_tiempo_distancia = 0;
int64_t ultimo_tiempo_temp = 0;

void app_main() {
    // Configuración inicial (WiFi, MQTT, ADC...)
    
    while (1) {
        int64_t ahora = esp_timer_get_time();

        // Tarea Rápida: Distancia (10 Hz -> cada 100.000 us)
        if (ahora - ultimo_tiempo_distancia >= 100000) {
            leer_sensor_distancia();
            enviar_mqtt_distancia();
            ultimo_tiempo_distancia = ahora;
        }

        // Tarea Lenta: Temperatura (0.1 Hz -> cada 10.000.000 us)
        if (ahora - ultimo_tiempo_temp >= 10000000) {
            leer_temperatura();
            enviar_mqtt_temperatura();
            ultimo_tiempo_temp = ahora;
        }

        // Pequeño delay para no saturar la CPU (Watchdog)
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

```

### 4.3.2. Medición de distancia (Sensor Sharp)

El sensor Sharp entrega un voltaje analógico. Utilizaremos el driver `adc_oneshot` para leerlo.

* **Reto:** La curva es no lineal ().
* **Implementación:** Debes aplicar la fórmula de conversión del datasheet o una tabla de búsqueda (Lookup Table) dentro de la función de lectura.

### 4.3.3. Cliente MQTT (ESP-MQTT)

Usaremos la librería nativa `mqtt_client`. Aunque nuestro código sea secuencial, esta librería maneja la conexión TCP por debajo (utilizando las interrupciones de red del sistema) para no bloquear nuestro bucle.

1. **Configuración:** Definir el *Broker* (`test.mosquitto.org`) y el puerto (1883).
2. **Event Loop:** El cliente MQTT requiere registrar un "Callback" o manejador de eventos para saber cuándo se ha conectado correctamente antes de empezar a enviar datos.

---

## 4.4. Descripción de la Integración Final

El alumno deberá entregar un programa en C estructurado de la siguiente forma:

### 1. Inicialización

* Iniciar la Flash (NVS) necesaria para el WiFi.
* Conectarse a la red WiFi (esperar a tener IP).
* Iniciar el ADC para el sensor Sharp.
* Iniciar y arrancar el cliente MQTT.

### 2. Bucle Principal (Super-Loop)

El `app_main` entrará en un bucle infinito donde se verificará el paso del tiempo:

* **Cada 100 ms (10 Hz):**
* Leer el valor ADC del pin del Sharp.
* Convertir a centímetros.
* Publicar en el topic `alumno/museo/distancia`.
* *(Opcional)* Si la distancia es < 10cm, imprimir "ALERTA" por consola.


* **Cada 10 segundos (0.1 Hz):**
* Leer temperatura (o generar valor simulado).
* Publicar en el topic `alumno/museo/temperatura`.



### 3. Verificación con Mosquitto

Desde un PC externo, se verificará el funcionamiento suscribiéndose a los topics:

```bash
mosquitto_sub -h test.mosquitto.org -t "alumno/museo/#" -v

```

Se deberá observar cómo los datos de distancia llegan a gran velocidad, intercalándose ocasionalmente con los datos de temperatura, demostrando que el bucle único es capaz de gestionar ambas tareas sin bloquearse.

---

## 4.5. Consideraciones Importantes

* **No usar `sleep()` largos:** En este modelo, está terminantemente prohibido usar funciones como `vTaskDelay(10000)` (esperar 10 segundos) dentro del `while(1)`, ya que eso detendría también la lectura del sensor de distancia. Todo el control de tiempo debe ser mediante comparación de timestamps (`if (ahora - antes > periodo)`).
* **Simplicidad:** Esta arquitectura evita problemas de "condiciones de carrera" (varios hilos tocando la misma variable a la vez) ya que el código se ejecuta línea a línea en orden.