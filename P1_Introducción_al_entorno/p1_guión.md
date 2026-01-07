

# Guión Práctica 1: Introducción al entorno
## Guía de Instalación de ESP-IDF sobre VS Code

En esta práctica se introduce el entorno de desarrollo **ESP-IDF** (Espressif IoT Development Framework), el framework oficial proporcionado por Espressif para la programación de microcontroladores de la familia ESP32. ESP-IDF permite desarrollar aplicaciones de bajo nivel en **C/C++**, ofreciendo un control completo sobre el hardware y los periféricos de la placa, y es utilizado tanto en entornos profesionales como educativos.

Para trabajar con ESP-IDF de forma más cómoda, se emplea **Visual Studio Code (VS Code)** como editor de código, junto con la extensión oficial de Espressif, que facilita el desarrollo, la compilación y la carga del código en la placa. Para poder utilizar esta extensión, es necesario disponer de algunas herramientas adicionales que garantizan el correcto funcionamiento del entorno.

En concreto, además de VS Code, es necesario instalar:

- **ESP-IDF Tools**: incluyen el compilador, el sistema de construcción, Python y otras utilidades necesarias para compilar y flashear el código en el microcontrolador.  
- **Git**: sistema de control de versiones utilizado para descargar el framework ESP-IDF y gestionar los repositorios del proyecto.  

Esta guía detalla los pasos necesarios para instalar y configurar correctamente el entorno de desarrollo, de modo que quede listo para su uso tanto en las prácticas como en el desarrollo del proyecto final de la asignatura.

Además, se va a realizar una pequeña comparación entre Raspberry Pi y ESP32-C3 previa a la guía de instalación para conocer sus características principales. 

Al abordar el desarrollo de prácticas originalmente diseñadas para Raspberry Pi y adaptarlas a un ESP32-C3, es útil comprender las diferencias arquitectónicas y de uso entre ambas plataformas, ya que esto condiciona el diseño del software y el acceso a periféricos.

## 1. Arquitectura y sistema operativo

**Raspberry Pi**

- Es una computadora completa embebida, basada en procesadores ARM más potentes.

- Normalmente ejecuta un sistema operativo multitarea (Linux).

- Permite múltiples procesos, administración de memoria compleja y uso de servicios de alto nivel.

- Su entorno de desarrollo puede usar APIs de alto nivel (Python, Bash, C con librerías del sistema, etc.).

**ESP32-C3**

- Es un microcontrolador de propósito específico, con recursos de hardware más limitados (RAM, CPU).

- No ejecuta un sistema operativo completo por defecto; típicamente usa RTOS embebido (FreeRTOS) o se programa “bare-metal”.

- El acceso al hardware se hace de forma directa o mediante drivers de bajo nivel (p. ej. ESP-IDF).

- Menor complejidad de software, mayor control temporal.


En resumen, Raspberry Pi actúa como una mini-computadora con varios cores mientras que eSP32-C3 es un microcontrolador enfocado en tareas de control en tiempo real.

## 2. Acceso a hardware y periféricos

**Raspberry Pi**

- Posee un conjunto de GPIOs de propósito general, accesibles mediante el sistema operativo.

- No siempre hay acceso directo a registros de hardware sin usar librerías.

**ESP32-C3**

- Permite acceso directo a registros de hardware GPIO, temporizadores y periféricos como PWM (LEDC), I2C, ADC, etc.

- El control de tiempo real es preciso: retardos en microsegundos, interrupciones, control por hardware.

Por estos motivos, ESP32-C3 ofrece un mayor control de hardware y Raspberry Pi requiere abstracción por el sistema operativo.

## 3. Consumo y eficiencia

**Raspberry Pi**

- Consume más energía (500 mA – 1 A típicamente).

- Requiere fuente de alimentación estable (fuente de problemas experimentados durante la realización de las prácticas).

**ESP32-C3**

- Diseñado para bajo consumo ( mA en funcionamiento, µA en modo sleep).

Por lo que ESP32-C3 es mucho más eficiente energéticamente.

## 4. Conectividad

**Raspberry Pi**

- Ethernet, Wi-Fi (en modelos), Bluetooth (en modelos), puertos USB, HDMI.

**ESP32-C3**

- Wi-Fi y Bluetooth LE integrados, lo que lo hace perfecto para IoT.

- No tiene puertos de alto nivel como USB/HDMI, SATA, etc.

## 5. Desarrollo de software

**Raspberry Pi**

- Programación de alto nivel (Python, C, C++, Java, etc.).

- Dispone de depuración avanzada, herramientas GNU completas.

**ESP32-C3**

- Se programa típicamente con ESP-IDF o frameworks como Arduino (también SDK).

- Requiere gestión de memoria y periféricos.

- Desarrollo más cercano al hardware.


En resumen, Raspberry Pi ofrece muchas funciones y facilidades, pero con un mayor consumo y menor control; ESP32-C3 exige mayor control pero da más precisión temporal.

## Tabla resumen: Raspberry Pi vs ESP32-C3

| Característica            | Raspberry Pi              | ESP32-C3                     |
|---------------------------|---------------------------|-------------------------------|
| Tipo de sistema           | Mini-computadora          | Microcontrolador              |
| Sistema operativo         | Linux (sí)                | No / FreeRTOS                 |
| Acceso hardware           | Abstracción del S.O.      | Directo / registros           |
| Tiempo real               | Difícil                   | Muy preciso                   |
| Consumo                   | Alto                      | Muy bajo                      |
| Periféricos               | Completo                  | Enfocado a IoT                |
| Conectividad              | Ethernet, USB, HDMI       | Wi-Fi, BLE                    |
| Aplicaciones típicas      | Multimedia, servidores    | Control embebido, sensores    |


---

### Guía de instalación de ESP-IDF Tools

Para iniciar la instalación, se debe acceder a la página oficial de Espressif Systems, donde se encuentra el instalador correspondiente al sistema operativo utilizado (Windows, Linux o macOS). Se recomienda descargar siempre la versión más reciente de ESP-IDF Tools para garantizar compatibilidad con los dispositivos más recientes y aprovechar las correcciones de errores presentes en versiones anteriores. [Guía oficial de instalación de ESP-IDF para Windows](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/windows-setup.html)  

<p align="center">
  <img src="https://github.com/user-attachments/assets/a1e8f58c-a44a-44b1-bae9-de5c42b96652" width="600" alt="Instalador ESP-IDF"/>
</p>

A continuación, se procede a descargar el instalador offline de ESP-IDF correspondiente al sistema operativo de cada equipo. En el caso de esta práctica, el ordenador utilizado dispone de **Windows 11**, por lo que se selecciona la versión específica para este sistema.  

<p align="center">
  <img src="https://github.com/user-attachments/assets/292535d9-b0ca-405e-bd0c-7ee0140736db" width="600" alt="Selección de versión"/>
</p>

Durante el proceso de instalación, el instalador configura automáticamente las dependencias necesarias, como Python y las variables de entorno, simplificando el procedimiento para el usuario. Una vez finalizada la instalación, el sistema queda preparado para integrarse con VS Code mediante la extensión oficial de ESP-IDF, permitiendo trabajar directamente dentro del entorno de desarrollo.

La instalación correcta de ESP-IDF Tools es fundamental, ya que sin estas herramientas no sería posible compilar ni cargar el firmware en la placa ESP32, ni utilizar los ejemplos y librerías proporcionados por Espressif.

---

### Instalación de la extensión ESP-IDF en VS Code

Una vez instaladas las herramientas, se abre **Visual Studio Code** para proceder a la instalación de la extensión ESP-IDF. Para ello:

1. Acceder a la pestaña **“Extensions”** del editor.  
2. Introducir el nombre de la extensión en el buscador para localizarla e instalarla.  

<p align="center">
  <img src="https://github.com/user-attachments/assets/45609795-9231-4a56-9865-fd174bd9a9a0" width="400" alt="Extensiones VS Code"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/1f129249-6507-42c2-9230-fcf143099732" width="600" alt="Búsqueda extensión"/>
</p>

Una vez instalada, la extensión aparecerá en la barra lateral izquierda de VS Code. Antes de comenzar a crear proyectos, es necesario realizar la **configuración inicial**, guiada de forma sencilla por el propio entorno. Durante este proceso se debe indicar la ruta de la carpeta donde se encuentran los archivos necesarios para la compilación y carga del código en la placa, es decir, los frameworks y herramientas descargados previamente desde la página oficial de Espressif.

<p align="center">
  <img src="https://github.com/user-attachments/assets/2af1c9f6-43b0-4ec9-8500-7a8d2db98344" width="400" alt="Configuración inicial"/>
</p>

Al hacer doble clic sobre la opción **“Configure ESP-IDF Extension”**, se accede a la configuración guiada de la extensión. En la ventana emergente se muestran varias opciones de configuración; en este caso, se selecciona **“Express”**, ya que no es necesario ajustar parámetros avanzados. A continuación, se debe elegir el dispositivo con el que se va a trabajar y, finalmente, indicar la ruta de la carpeta **“tools”** previamente descargada, para que la extensión pueda utilizar los archivos necesarios para la compilación y carga del código en la placa.

<p align="center">
<img width="600" alt="image" src="https://github.com/user-attachments/assets/435a73be-6749-47b7-bec7-a41e92d281d0" />

</p>

De este modo, se instalan todos los archivos necesarios para trabajar con el dispositivo desde VS Code, un proceso que puede tardar varios minutos. Una vez completada la instalación, ya es posible crear un proyecto en blanco y comenzar a familiarizarse con la programación a bajo nivel en este tipo de dispositivos.

<p align="center">
  <img src="https://github.com/user-attachments/assets/776dc87e-31a7-4280-a09a-89a899b44a0b" width="600" alt="Entorno listo"/>
</p>

Para seguir paso a paso la configuración del entorno en Visual Studio Code y ver un ejemplo básico de funcionamiento, se recomiendan los siguientes vídeos, donde se explica el proceso de instalación, configuración y verificación de la placa. 

El segundo vídeo es una actualización del primero, ya que este contiene algunos pasos o datos que han quedado desfasados:

- `https://www.youtube.com/watch?v=5IuZ-E8Tmhg&t=57s`
- `https://www.youtube.com/watch?v=N93RvZz6dEc`


---

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

- el manejo directo de los registros GPIO del ESP32-C3,

- la generación de señales periódicas por software,

- la relación entre frecuencia, período y duración de una señal,

y la síntesis básica de sonido mediante un zumbador.

Además, constituye una base sólida para una futura implementación de un piano digital, donde la generación de sonido podría migrarse a un sistema PWM por hardware para mejorar la eficiencia y reducir la carga de la CPU.


---

