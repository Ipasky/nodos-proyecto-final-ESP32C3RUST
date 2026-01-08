

# Guión Práctica 1: Introducción al entorno
## Primer ejemplo


Las prácticas originales están diseñadas para ejecutarse sobre una Raspberry Pi, haciendo uso de sus periféricos y del sistema operativo subyacente. Por este motivo, su adaptación al ESP32-C3 requiere ciertas modificaciones, ya que este microcontrolador no dispone de un sistema operativo completo ni de los mismos periféricos externos que una Raspberry Pi.

Con el objetivo de facilitar esta transición, se plantea un primer ejemplo introductorio orientado a familiarizarse con la programación a bajo nivel del ESP32-C3, utilizando el entorno ESP-IDF y accediendo directamente a los registros de hardware. Este ejercicio permite asimilar conceptos básicos como la configuración de pines de entrada y salida, el flasheo del dispositivo y la ejecución de código en la placa.

El primer paso consiste en crear un proyecto básico en el que se pueden añadir archivos fuente en lenguaje C según las necesidades de la aplicación.

<p align="center"> <img width="600" alt="image" src="https://github.com/user-attachments/assets/d2e0f7a0-670a-4284-a132-e36afb2c3af7" /> <img width="600" alt="image" src="https://github.com/user-attachments/assets/7c6e10c0-b680-417f-92e0-2a30cb04226d" /> </p>

A partir de la documentación del fabricante, se identifican los GPIOs disponibles y las direcciones de los registros de control asociados. Con esta información, se ha desarrollado un código sencillo que permite controlar el brillo de un LED conectado a un GPIO, como introducción práctica a la modulación por ancho de pulso (PWM).

En esta implementación, la señal PWM se genera por software, sin utilizar el periférico PWM por hardware del ESP32-C3. Para ello, se accede directamente a los registros GPIO mediante punteros a memoria, lo que permite forzar el pin a nivel alto o bajo con gran precisión temporal.

El pin del LED se configura como salida activando el bit correspondiente en el registro de habilitación:
```c
*gpio_enable_w1ts = (1 << LED_PIN);
```

La modulación PWM se implementa mediante dos bucles anidados. El bucle externo ajusta progresivamente el duty cycle, mientras que el bucle interno genera un período completo de la señal PWM encendiendo o apagando el LED en función del valor del duty:
```c
if (i < duty)
    *gpio_out_w1ts = (1 << LED_PIN); // LED encendido
else
    *gpio_out_w1tc = (1 << LED_PIN); // LED apagado
```

Los retardos necesarios para definir la frecuencia de la señal se generan mediante la función ets_delay_us(), que introduce pausas en microsegundos:
```c
ets_delay_us(PWM_DELAY_US);
```

El programa incrementa gradualmente el duty cycle desde 0 hasta su valor máximo, aumentando progresivamente el brillo del LED, y posteriormente lo decrementa, produciendo un efecto de atenuación suave.

Este ejemplo introductorio permite comprender:

- la configuración y control directo de GPIOs,

- la generación de señales PWM por software,

- la relación entre resolución, frecuencia y duty cycle,

y el acceso a registros hardware en el ESP32-C3.

Además, sienta las bases necesarias para la implementación de PWM por hardware en prácticas posteriores, así como para el desarrollo de aplicaciones más complejas de control y generación de señales.

---
## Práctica 1
# Generación de notas musicales en el ESP32-C3 mediante control software de GPIO

## 1. Introducción

El código desarrollado tiene como objetivo **generar señales cuadradas de distintas frecuencias mediante control software de un GPIO del ESP32-C3**, con el fin de **reproducir notas musicales en un zumbador**.  
Este programa constituye la base para una futura implementación de un **piano digital**, donde cada nota se asocia a su frecuencia fundamental.

La generación de la señal se realiza **sin utilizar el periférico PWM por hardware (LEDC)**, sino controlando directamente los registros GPIO, lo que permite comprender el funcionamiento a bajo nivel del microcontrolador.

---

## 2. Inclusión de librerías y definición de registros GPIO

Se incluyen las librerías estándar necesarias para el manejo de tipos de datos, cadenas de caracteres y retardos a nivel de ROM:

```c
#include <stdint.h>
#include <string.h>
#include "esp32c3/rom/ets_sys.h"
````
A continuación, se definen las direcciones base de los registros GPIO del ESP32-C3 y se crean punteros para acceder directamente a ellos:

<p align="center">

<img width="400" alt="image" src="https://github.com/user-attachments/assets/ee7cbcf6-0a17-442d-a8d2-438c65caa041" />
</p>
<p align="center">
<img width="500" alt="image" src="https://github.com/user-attachments/assets/302e5f9d-8e9b-4a05-8654-f048e175d05b" />
</p>

```c
#define GPIO_BASE        0x60004000
#define GPIO_OUT_W1TS    (GPIO_BASE + 0x08)
#define GPIO_OUT_W1TC    (GPIO_BASE + 0x0C)
#define GPIO_ENABLE_W1TS (GPIO_BASE + 0x20)

#define ZUMBADOR 2 
```

Estos registros permiten:

- Configurar un pin como salida.

- Forzar un pin a nivel alto.

- Forzar un pin a nivel bajo.
```c
volatile uint32_t* gpio_out_w1ts = (volatile uint32_t*) GPIO_OUT_W1TS;
volatile uint32_t* gpio_out_w1tc = (volatile uint32_t*) GPIO_OUT_W1TC;
volatile uint32_t* gpio_enable_w1ts = (volatile uint32_t*) GPIO_ENABLE_W1TS;
```
## 3. Definición de estructuras de datos
### 3.1 Estructura de notas musicales

Se define una estructura Nota que asocia el nombre de la nota con su frecuencia fundamental en hercios:
```c
typedef struct {
    const char *nombre;
    int freq;   // frecuencia en Hz
} Nota;

```
Se crea un array con las notas musicales básicas:
```c
Nota notas[] = {
    {"DO", 261},
    {"RE", 294},
    {"MI", 329},
    {"FA", 349},
    {"SOL", 392},
    {"LA", 440},
    {"SI", 493},
    {"DO5", 523}
};
```
### 3.2 Estructura de la partitura

Para definir la melodía a reproducir, se utiliza la estructura NotaPartitura, que asocia cada nota con su duración:
```c
typedef struct {
    const char *nombre;
    int duracion_ms;
} NotaPartitura;

```
La partitura se define como un array de notas y duraciones:
```c
NotaPartitura partitura[] = {
    {"DO", 400}, {"DO", 400}, {"SOL", 400}, {"SOL", 400},
    {"LA", 400}, {"LA", 400}, {"SOL", 800},
    {"FA", 400}, {"FA", 400}, {"MI", 400}, {"MI", 400},
    {"RE", 400}, {"MI", 400}, {"DO", 800}
};
```
## 4. Búsqueda de notas por nombre

La función **Indice()** permite buscar una nota por su nombre dentro del array notas[] y devolver su índice:
```c
int Indice(const char *nombre) {
    for (int i = 0; i < numNotas; i++) {
        if (strcmp(nombre, notas[i].nombre) == 0) return i;
    }
    return -1;
}

```
Esta función facilita la obtención de la frecuencia correspondiente a cada nota musical.

## 5. Función de retardo

La función **delay_ms()** implementa retardos en milisegundos utilizando retardos en microsegundos de la ROM del ESP32-C3:
```c
void delay_ms(int t) {
    ets_delay_us(t * 1000);
}
```

Se emplea para introducir pausas entre notas y entre repeticiones de la melodía.

## 6. Generación de las notas musicales

La función **play_nota()** es la encargada de generar la señal cuadrada correspondiente a una nota musical concreta:
```c
void play_nota(const char *nota, int duracion_ms) {
    int freq = notas[Indice(nota)].freq;
    if (freq <= 0) return;
```

A partir de la frecuencia se calcula el período de la señal y se establece un duty cycle del 50 %:
```c 
    int periodo_us = 1000000 / freq;
    int mitad_periodo = periodo_us / 2;
    int repeticiones = (duracion_ms * 1000) / periodo_us;
```

El GPIO se conmuta entre nivel alto y bajo para generar la señal:
```c
    for (int i = 0; i < repeticiones; i++) {
        *gpio_out_w1ts = (1 << ZUMBADOR);
        ets_delay_us(mitad_periodo);

        *gpio_out_w1tc = (1 << ZUMBADOR);
        ets_delay_us(mitad_periodo);
    }

    delay_ms(50);
}
```
## 7. Función principal app_main()

En la función principal se configura el pin del zumbador como salida:
```c
void app_main(void) {
    *gpio_enable_w1ts = (1 << ZUMBADOR);
```

A continuación, se reproduce la partitura de forma continua dentro de un bucle infinito:
```c
    while (1) {
        for (int i = 0; i < longPartitura; i++) {
            play_nota(partitura[i].nombre, partitura[i].duracion_ms);
        }

        delay_ms(200);
    }
}
```
## 8. Conclusión

Este código permite comprender:

- el manejo directo de los registros GPIO del ESP32-C3.

- la generación de señales periódicas por software.

- la relación entre frecuencia, período y duración de una señal.

y la síntesis básica de sonido mediante un zumbador.

Gracias a este código se consigue generar un piano digital mediante la generación de señales PWM. 


---

