# Guión Práctica 3: Sensores y actuadores con GPS (NEO-7M) en ESP32-C3

## Adaptación de la Práctica 3 a la plataforma ESP32-C3

En esta práctica se trabaja la integración de sensores y actuadores en un sistema embebido, cuyo objetivo principal es cerrar el lazo de control: adquisición de datos del entorno, procesamiento de la información y actuación sobre el mundo físico.

La práctica original estaba diseñada para realizarse sobre una Raspberry Pi, utilizando una unidad de medición inercial (IMU) LSM6DS33 conectada por bus I2C para obtener la orientación (giro, cabeceo y alabeo). A partir de estos datos, se actuaba sobre un servomotor mediante señales PWM generadas por software o hardware a través de la librería WiringPi.

Sin embargo, para la realización de esta práctica en la plataforma ESP32-C3, se ha decidido adaptar el contenido sustituyendo el sensor inercial por un receptor GPS. Esta modificación implica cambios significativos en los buses de comunicación y en la naturaleza de los datos procesados, pero mantiene intacto el objetivo didáctico de vincular una magnitud física de entrada (orientación/rumbo) con una acción mecánica de salida.

## Cambios en la plataforma hardware

La ESP32-C3 dispone de múltiples interfaces de comunicación serie, lo que facilita la integración de diversos sensores. En la práctica original se utilizaba el bus **I2C** para leer aceleraciones. En esta adaptación, al utilizar un módulo GPS NEO-7M, se pasa a utilizar el protocolo **UART (Universal Asynchronous Receiver-Transmitter)**.

Esta transición implica un cambio en el paradigma de comunicación:

* **I2C (Original):** Comunicación síncrona maestro-esclavo, direccionable por registros.
* **UART (Adaptación):** Comunicación asíncrona punto a punto, basada en flujo de datos continuo (stream).

Por otro lado, la actuación sobre el servomotor en la Raspberry Pi solía depender de la gestión del sistema operativo o librerías externas para generar el PWM. La ESP32-C3 integra un periférico dedicado llamado **LEDC (LED Control)**, diseñado específicamente para generar señales PWM con alta precisión y sin carga para la CPU, ideal para el control de servos.

Por estos motivos, la práctica se centra en:

* El uso del periférico **UART** para la recepción y parseo de tramas NMEA estándar.
* El uso del periférico **LEDC** para la generación de señales PWM de 50 Hz.
* La transformación del dato de **"Rumbo" (Course over Ground)** del GPS en una posición angular para el servo.

## Cambios en el entorno software

Mientras que la práctica original se apoyaba en WiringPi y en la lectura de registros I2C byte a byte, en esta adaptación se emplea **ESP-IDF**.

Para la lectura del GPS se utiliza el driver **UART**, configurado para operar mediante eventos o *polling* del buffer de recepción. Dado que el GPS envía información de forma constante (normalmente una vez por segundo), el software debe ser capaz de detectar el inicio y fin de las tramas de texto.

Para sustituir la lectura de inclinación (Roll/Pitch) del acelerómetro, se utiliza el dato de **Rumbo** (Heading/Track angle) proporcionado por el GPS en las tramas `$GNRMC`. De esta forma, se mantiene la lógica de la práctica:

* **Original:** Inclinación del sensor  Posición del Servo.
* **Adaptación:** Orientación geográfica (Rumbo)  Posición del Servo.

Finalmente, se implementa una tarea de actuación que mapea el ángulo de 0º a 360º del rumbo GPS al rango de 0º a 180º del servomotor, actualizando el ciclo de trabajo (duty cycle) del periférico LEDC.

## Configuración de los periféricos en la ESP32-C3

La ESP32-C3 gestiona sus periféricos mediante registros mapeados en memoria. Tanto la UART como el controlador LEDC disponen de bloques de registros complejos que controlan la temporización, las interrupciones y el flujo de datos.

El acceso directo a estos registros permite una optimización máxima, pero conlleva una gran complejidad en la gestión de relojes y estados. A continuación, se describen los registros que intervienen en el proceso, aunque posteriormente se utilizarán drivers para su manejo.

---

### Registros principales del periférico UART

#### UART_FIFO_REG

Es el registro de acceso a la memoria FIFO (First In, First Out) de transmisión y recepción. Al leer de este registro, se extraen los bytes recibidos desde el GPS. Al escribir, se envían datos. La ESP32-C3 dispone de un buffer hardware que permite almacenar bytes mientras la CPU realiza otras tareas.

---

#### UART_INT_ENA_REG / UART_INT_RAW_REG

Estos registros controlan las interrupciones. En el caso del GPS, son vitales las interrupciones de `RXFIFO_FULL` (buffer lleno) o `RXFIFO_TOUT` (tiempo de espera excedido tras recibir datos), que indican al procesador que hay una trama nueva lista para ser procesada.

---

#### UART_CLKDIV_REG

Define el divisor de frecuencia para generar el *Baud Rate* (velocidad de transmisión). Dado que el GPS NEO-7M transmite típicamente a 9600 baudios, este registro debe ajustarse con precisión a partir del reloj base del sistema (APB Clock) para evitar errores de paridad o trama.

---

#### UART_CONF0_REG / UART_CONF1_REG

Configuran los parámetros de la trama: bits de datos (8), paridad (None) y bits de parada (1). Una desconfiguración aquí provocaría la recepción de caracteres ininteligibles ("basura").

---

### Registros principales del periférico LEDC (PWM)

#### LEDC_LSCHn_CONF_REG (Low Speed Channel Config)

Configura el canal específico del PWM. En la ESP32-C3, a diferencia de la ESP32 original, solo existe el modo "Low Speed". Aquí se define la asociación entre el temporizador y el canal de salida hacia el GPIO del servo.

---

#### LEDC_LSTIMERn_CONF_REG

Configura el temporizador base. Define la frecuencia de la señal PWM. Para un servomotor estándar, este registro debe configurarse para obtener una frecuencia de **50 Hz** (periodo de 20 ms). También define la resolución en bits (ej. 14 bits), lo que determina la precisión del movimiento.

---

#### LEDC_LSCHn_DUTY_REG

Es el registro donde se escribe el valor del ciclo de trabajo (*duty cycle*).

* Un valor correspondiente a **1 ms** posiciona el servo en 0º.
* Un valor correspondiente a **2 ms** posiciona el servo en 180º.
El valor numérico a escribir depende de la resolución configurada en el timer.

---

## Justificación del uso de los drivers `uart` y `ledc`

Aunque es posible configurar la UART y el PWM escribiendo directamente en los registros mencionados (`UART_CLKDIV_REG`, `LEDC_LSTIMERn_CONF_REG`, etc.), esto requiere cálculos manuales de los divisores de reloj y una gestión compleja de las colas FIFO y las interrupciones.

Por este motivo, y para centrar la práctica en la lógica de control y el parseo de protocolos estándar, se ha optado por utilizar los drivers de alto nivel proporcionados por ESP-IDF. Estos drivers abstraen la capa física y garantizan una configuración robusta de los relojes y los pines.

### Inicialización del GPS (UART)

Para la lectura del GPS se utiliza el driver **UART**, que gestiona internamente un *Ring Buffer* en software, permitiendo leer tramas completas sin perder bytes.

```c
void gps_uart_init(void)
{
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    // Configuración de parámetros y pines (TX=4, RX=5)
    uart_param_config(UART_NUM_1, &cfg);
    uart_set_pin(UART_NUM_1, 4, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Instalación del driver con buffer de 2048 bytes
    uart_driver_install(UART_NUM_1, 2048, 0, 0, NULL, 0);
}

```

En este código, `uart_param_config` calcula automáticamente los valores para los registros de división de reloj, y `uart_driver_install` reserva la memoria y configura las interrupciones necesarias para llenar el buffer en segundo plano.

### Inicialización del Servo (LEDC)

Para el servomotor, el driver **LEDC** permite definir la frecuencia y la resolución de forma declarativa.

```c
void servo_init(void)
{
    // Configuración del Timer: 50 Hz, 14 bits de resolución
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_14_BIT,
        .freq_hz          = 50,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    // Configuración del Canal: GPIO 6
    ledc_channel_config_t channel = {
        .gpio_num   = 6,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0, // Inicialmente en 0
        .hpoint     = 0
    };
    ledc_channel_config(&channel);
}

```

Esta abstracción permite al estudiante centrarse en calcular el *duty cycle* necesario para el ángulo deseado, en lugar de configurar los prescalers del hardware.

---

## Análisis de los resultados

Una vez cargado el código en la ESP32-C3, el sistema comienza a recibir el flujo de datos NMEA del GPS. Dado que el GPS puede tardar unos minutos en obtener una posición válida (*FIX*), el software debe discriminar entre tramas válidas y vacías.

En una primera etapa, se visualizan las tramas **$GNRMC** (Recommended Minimum Navigation Information) a través del puerto serie de depuración (USB).

**Ejemplo de trama recibida:**
`$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,,,A*6C`

El software desarrollado parsea esta cadena de texto para extraer el campo de **Rumbo** (en el ejemplo: `084.4` grados). A diferencia del acelerómetro, que entrega un valor numérico directo en un registro, aquí es necesario convertir la cadena de caracteres ASCII a un valor en punto flotante (`float`).

A continuación, se realiza el mapeo de la magnitud física medida a la señal de control:

Se divide por 2 ya que el rumbo cubre 360º y el servomotor estándar solo cubre 180º.

### Caracterización del movimiento

Al desplazar el receptor GPS en línea recta en diferentes direcciones cardinales, se observa la respuesta del sistema:

1. **Norte (0º / 360º):** El servo se posiciona en su extremo izquierdo (aprox. 0º).
2. **Este (90º):** El servo se mueve a 45º.
3. **Sur (180º):** El servo se centra en 90º.
4. **Oeste (270º):** El servo se mueve a 135º.

Se ha verificado que la señal PWM generada por el periférico LEDC mantiene una frecuencia estable de 50 Hz. Sin embargo, al igual que ocurría con la no idealidad del ADC en la práctica anterior, el servomotor mecánico presenta limitaciones físicas.

| Rumbo GPS (Entrada) | Ángulo Teórico Servo | Ancho de Pulso (ms) | Observación |
| --- | --- | --- | --- |
| 0º (Norte) | 0º | 1.0 ms | El servo puede tener una zona muerta al inicio. |
| 180º (Sur) | 90º | 1.5 ms | Posición central, máxima estabilidad. |
| 359º (Norte) | 179.5º | ~2.0 ms | El servo puede vibrar si se fuerza el tope mecánico. |

En conclusión, la adaptación a la ESP32-C3 ha permitido replicar la cadena de **Sensor  Proceso  Actuador** utilizando periféricos modernos (UART y LEDC) y un protocolo de comunicación industrial (NMEA), sustituyendo con éxito la funcionalidad de la práctica original basada en I2C.