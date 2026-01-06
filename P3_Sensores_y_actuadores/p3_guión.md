# Guión Práctica 3: Sensores y actuadores con **GPS (NEO-7M)** en **ESP32-C3**

## Adaptación de la Práctica 3 a la plataforma ESP32-C3

La práctica original estaba planteada para una **Raspberry Pi** y una unidad inercial **LSM6DS33** (acelerómetro + giróscopo) conectada por **I2C**, además de un **servomotor por PWM** y el uso de **LEDs para representar temperatura**. El objetivo final era integrar lectura de sensores y actuación sobre un servo/LEDs en una única aplicación. 

En esta adaptación se sustituye la unidad inercial por un **módulo GPS NEO-7M**, porque no se dispone del giroscopio/acelerómetro. Se mantiene el espíritu de la práctica (sensor → procesamiento → actuador + consola) cambiando:
- **Bus serie**: de **I2C** (LSM6DS33) a **UART** (GPS).
- **Magnitud “de orientación”**: en lugar de roll/pitch/yaw con acelerómetro, se utiliza el **rumbo (course over ground)** que entrega el GPS (cuando hay FIX).
- **Actuación**: se conserva el **servomotor por PWM** (ahora usando el periférico **LEDC** de ESP-IDF).  
> El servomotor sigue funcionando con una señal PWM típica de **20 ms de período (50 Hz)**, igual que se razonaba en la práctica original. 

---

## Cambios en la plataforma hardware

### Material utilizado
- Placa **ESP32-C3-DevKit-Rust-v1.2**
- Módulo **GPS NEO-7M** (UART, típico 9600 baudios)
- **Servomotor** (5 V) + cables Dupont
- (Opcional) LED externo o LED onboard para estado FIX / NO FIX

### Conexiones GPS ↔ ESP32-C3 (UART)
El GPS entrega datos por UART a 9600 baudios normalmente. Se conectan 4 señales:

- **GPS TX → ESP32 RX** (pin RX que elijas en el código)
- **GPS RX → ESP32 TX** (pin TX que elijas en el código)
- **GPS GND → ESP32 GND**
- **GPS VCC → 3V3 o 5V según tu placa GPS**
  - Importante: muchos módulos GPS tienen regulador y aceptan 5 V, pero **la lógica UART suele ser 3.3 V**. Si tu módulo trabaja a 5 V lógico, usa conversión de nivel.

> En ESP32-C3 puedes enrutar UART a varios GPIO. En este guión se propone **UART1** con pines configurables en el código para no interferir con el puerto de programación/monitor.

### Conexión servomotor (PWM)
- **Señal (blanco/amarillo) → GPIO de salida PWM (LEDC)**
- **GND (negro/marrón) → GND común**
- **VCC (rojo) → 5 V**
> El servo puede consumir bastante corriente, así que si se comporta raro, usa fuente 5 V externa y une tierras. :contentReference[oaicite:2]{index=2}

---

## Cambios en el entorno software

La práctica original se apoyaba en **WiringPi** y comandos de Raspberry (I2C, PWM, etc.).   
En esta adaptación se utiliza **ESP-IDF** (framework oficial de Espressif). Para implementar la práctica se usan:
- **Driver UART** para leer el flujo NMEA del GPS (UART events / lectura por bytes). :contentReference[oaicite:4]{index=4}
- **Driver LEDC** para generar PWM estable a 50 Hz para el servo. :contentReference[oaicite:5]{index=5}
- **FreeRTOS** para separar lectura (GPS) y actuación (servo/LED/log).

---

## Estructura de directorios del proyecto (ESP-IDF)

Estructura mínima recomendada (similar a como se organiza en ESP-IDF y coherente con el estilo de la práctica 2):

practica3_gps/
├─ CMakeLists.txt
├─ main/
│ ├─ CMakeLists.txt
│ └─ main.c
└─ scripts/ (opcional)
└─ log_serial.py (opcional: captura por puerto serie)


**¿Qué tienes que crear/cambiar para un código simple?**
- Crear un proyecto con `idf.py create-project`
- En `main/`:
  - escribir tu código en `main.c`
  - asegurarte de que `main/CMakeLists.txt` registra el archivo
- (Opcional) scripts en Python van fuera para capturar/loguear, pero **no son necesarios para compilar**.

---

## Descripción de la práctica (integración final) adaptada a GPS

La aplicación final (entregable) integra:

1. **Fase inicial (adquisición GPS)**: durante unos segundos se espera a recibir tramas válidas (FIX).  
2. **Muestreo continuo (≈ 1 Hz)**: se leen tramas NMEA, se extraen:
   - latitud, longitud (si hay FIX)
   - velocidad (si aparece)
   - **rumbo/curso** (si aparece)  
3. **Actuación**:
   - El **servo** se posiciona en función del rumbo (0°–180°).
   - Por consola se imprime el estado y valores.
   - (Opcional) LED: FIX = encendido, NO FIX = parpadeo.
4. **Reset (opcional)**: si se pulsa un botón, se vuelve a “estado inicial” (servo centrado y espera de FIX).  
> En la práctica original también existía una lógica de reinicio del sistema. :contentReference[oaicite:6]{index=6}

---

## UART + GPS: qué estamos leyendo (NMEA)

El GPS suele enviar frases NMEA tipo:
- `$GPRMC` / `$GNRMC`: recomendado para **curso (track), velocidad y validez**
- `$GPGGA` / `$GNGGA`: recomendado para **calidad de fix y altitud**

En esta adaptación se parsea **RMC** por simplicidad (hay ejemplos de parsing en C/ESP-IDF). :contentReference[oaicite:7]{index=7}

### Conversión típica de coordenadas NMEA a grados decimales

En NMEA, una coordenada suele venir como:
- latitud: `ddmm.mmmm`
- longitud: `dddmm.mmmm`

Conversión:

$$
\text{grados\_dec} = \text{grados} + \frac{\text{minutos}}{60}
$$

---

## PWM de servo con LEDC (ESP32-C3)

El servo se controla con:
- **Frecuencia**: 50 Hz (período 20 ms) :contentReference[oaicite:8]{index=8}
- **Pulso**: típico 1 ms (0°), 1.5 ms (90°), 2 ms (180°)

Se implementa con **LEDC** (timer + channel). :contentReference[oaicite:9]{index=9}

---

## Código mínimo (ESP-IDF): leer GPS por UART y mostrar por consola

> Este ejemplo imprime por consola las líneas NMEA y extrae datos básicos si detecta una trama RMC.
> Luego, **mapea el rumbo a 0–180°** y mueve el servo.

### `main/main.c`
```c
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <math.h>

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"

    #include "driver/uart.h"
    #include "driver/ledc.h"
    #include "driver/gpio.h"
    #include "esp_log.h"

    static const char *TAG = "P3_GPS";

    // ======= Ajusta aquí tus pines =======
    #define GPS_UART            UART_NUM_1
    #define GPS_TX_GPIO         4   // ESP32 TX -> GPS RX
    #define GPS_RX_GPIO         5   // ESP32 RX <- GPS TX
    #define GPS_BAUDRATE        9600

    #define SERVO_GPIO          6   // Señal PWM del servo

    // ======= Servo (50 Hz) =======
    #define SERVO_FREQ_HZ       50
    #define SERVO_MIN_US        1000  // 1.0 ms
    #define SERVO_MAX_US        2000  // 2.0 ms
    #define SERVO_NEUTRAL_US    1500  // 1.5 ms

    // LEDC settings
    #define LEDC_TIMER          LEDC_TIMER_0
    #define LEDC_MODE           LEDC_LOW_SPEED_MODE
    #define LEDC_CHANNEL        LEDC_CHANNEL_0
    #define LEDC_DUTY_RES       LEDC_TIMER_14_BIT  // suficiente para servo

    static uint32_t servo_us_to_duty(int pulse_us)
    {
        // Duty = (pulse_us / period_us) * (2^res - 1)
        const int period_us = 1000000 / SERVO_FREQ_HZ; // 20000 us
        const int max_duty = (1 << 14) - 1;

        if (pulse_us < SERVO_MIN_US) pulse_us = SERVO_MIN_US;
        if (pulse_us > SERVO_MAX_US) pulse_us = SERVO_MAX_US;

        return (uint32_t)((pulse_us * (int64_t)max_duty) / period_us);
    }

    static void servo_set_angle(float angle_deg)
    {
        if (angle_deg < 0) angle_deg = 0;
        if (angle_deg > 180) angle_deg = 180;

        // mapeo lineal 0..180 -> 1000..2000 us
        int pulse = (int)(SERVO_MIN_US + (angle_deg / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US));
        uint32_t duty = servo_us_to_duty(pulse);

        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    }

    static void gps_uart_init(void)
    {
        uart_config_t cfg = {
            .baud_rate = GPS_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
        };

        ESP_ERROR_CHECK(uart_param_config(GPS_UART, &cfg));
        ESP_ERROR_CHECK(uart_set_pin(GPS_UART, GPS_TX_GPIO, GPS_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(GPS_UART, 2048, 0, 0, NULL, 0));
    }

    static void servo_init(void)
    {
        ledc_timer_config_t timer = {
            .speed_mode       = LEDC_MODE,
            .timer_num        = LEDC_TIMER,
            .duty_resolution  = LEDC_DUTY_RES,
            .freq_hz          = SERVO_FREQ_HZ,
            .clk_cfg          = LEDC_AUTO_CLK
        };
        ESP_ERROR_CHECK(ledc_timer_config(&timer));

        ledc_channel_config_t ch = {
            .gpio_num   = SERVO_GPIO,
            .speed_mode = LEDC_MODE,
            .channel    = LEDC_CHANNEL,
            .intr_type  = LEDC_INTR_DISABLE,
            .timer_sel  = LEDC_TIMER,
            .duty       = servo_us_to_duty(SERVO_NEUTRAL_US),
            .hpoint     = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ch));
    }

    // ---- Helpers NMEA ----
    static float nmea_degmin_to_decimal(const char *s, int is_lon)
    {
        // lat: ddmm.mmmm, lon: dddmm.mmmm
        double v = atof(s);
        int deg = is_lon ? (int)(v / 100.0) : (int)(v / 100.0);
        double min = v - (deg * 100.0);
        return (float)(deg + (min / 60.0));
    }

    static void parse_rmc(const char *line)
    {
        // Ejemplo:
        // $GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,,,A*6C
        // campos:      1     2    3      4  5       6  7     8     ...

        char buf[128];
        strncpy(buf, line, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';

        char *tok;
        int field = 0;

        char status = 'V';
        char lat_s[16] = {0}, ns = 'N';
        char lon_s[16] = {0}, ew = 'E';
        float speed_kn = 0.0f;
        float course_deg = -1.0f;

        tok = strtok(buf, ",");
        while (tok) {
            field++;

            switch (field) {
                case 2: // hora
                    break;
                case 3: // status A/V
                    status = tok[0];
                    break;
                case 4: // lat
                    strncpy(lat_s, tok, sizeof(lat_s)-1);
                    break;
                case 5: // N/S
                    ns = tok[0];
                    break;
                case 6: // lon
                    strncpy(lon_s, tok, sizeof(lon_s)-1);
                    break;
                case 7: // E/W
                    ew = tok[0];
                    break;
                case 8: // speed knots
                    speed_kn = (float)atof(tok);
                    break;
                case 9: // course deg
                    course_deg = (float)atof(tok);
                    break;
                default:
                    break;
            }

            tok = strtok(NULL, ",");
        }

        if (status != 'A') {
            ESP_LOGW(TAG, "RMC sin FIX (status=%c).", status);
            servo_set_angle(90);
            return;
        }

        float lat = nmea_degmin_to_decimal(lat_s, 0);
        float lon = nmea_degmin_to_decimal(lon_s, 1);
        if (ns == 'S') lat = -lat;
        if (ew == 'W') lon = -lon;

        float speed_kmh = speed_kn * 1.852f;

        ESP_LOGI(TAG, "FIX OK | Lat=%.6f Lon=%.6f | Speed=%.2f km/h | Course=%.1f deg",
                lat, lon, speed_kmh, course_deg);

        // Map course (0..360) -> servo (0..180)
        if (course_deg >= 0.0f) {
            float servo_angle = fmodf(course_deg, 360.0f) / 2.0f;
            servo_set_angle(servo_angle);
        }
    }

    static void gps_task(void *arg)
    {
        uint8_t c;
        char line[256];
        int idx = 0;

        while (1) {
            int n = uart_read_bytes(GPS_UART, &c, 1, pdMS_TO_TICKS(100));
            if (n <= 0) continue;

            if (c == '\n') {
                line[idx] = '\0';
                idx = 0;

                // Mostrar línea recibida (opcional)
                // printf("%s\n", line);

                if (strstr(line, "RMC")) {
                    parse_rmc(line);
                }
            } else if (c != '\r') {
                if (idx < (int)sizeof(line)-1) line[idx++] = (char)c;
            }
        }
    }

    void app_main(void)
    {
        ESP_LOGI(TAG, "Init UART GPS + Servo (LEDC)");
        gps_uart_init();
        servo_init();

        // Centro servo
        servo_set_angle(90);

        xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
    }
```