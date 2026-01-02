

# Guión Práctica 1: Introducción al entorno
## Guía de Instalación de ESP-IDF sobre VS Code

En esta práctica se introduce el entorno de desarrollo **ESP-IDF** (Espressif IoT Development Framework), el framework oficial proporcionado por Espressif para la programación de microcontroladores de la familia ESP32. ESP-IDF permite desarrollar aplicaciones de bajo nivel en **C/C++**, ofreciendo un control completo sobre el hardware y los periféricos de la placa, y es utilizado tanto en entornos profesionales como educativos.

Para trabajar con ESP-IDF de forma más cómoda, se emplea **Visual Studio Code (VS Code)** como editor de código, junto con la extensión oficial de Espressif, que facilita el desarrollo, la compilación y la carga del código en la placa. Para poder utilizar esta extensión, es necesario disponer de algunas herramientas adicionales que garantizan el correcto funcionamiento del entorno.

En concreto, además de VS Code, es necesario instalar:

- **ESP-IDF Tools**: incluyen el compilador, el sistema de construcción, Python y otras utilidades necesarias para compilar y flashear el código en el microcontrolador.  
- **Git**: sistema de control de versiones utilizado para descargar el framework ESP-IDF y gestionar los repositorios del proyecto.  

Esta guía detalla los pasos necesarios para instalar y configurar correctamente el entorno de desarrollo, de modo que quede listo para su uso tanto en las prácticas como en el desarrollo del proyecto final de la asignatura.

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

---

## Primer ejemplo

Las prácticas originales están diseñadas para Raspberry Pi, utilizando sus periféricos, por lo que su implementación en ESP32-C3 puede presentar algunas variaciones, ya que este microcontrolador no dispone de todas las funcionalidades ni de los elementos externos de la Raspberry Pi.

Por ello, se plantea un primer ejemplo introductorio para familiarizarse con la programación a bajo nivel en ESP-IDF. Este ejercicio sencillo permite comprender conceptos básicos como la configuración de pines de entrada y salida, el flasheado del dispositivo y la carga del código en la placa.

El primer paso consiste en crear un proyecto en el que se pueden añadir archivos .c según las necesidades de la aplicación.

<p align="center"> <img width="600" alt="image" src="https://github.com/user-attachments/assets/d2e0f7a0-670a-4284-a132-e36afb2c3af7" /> <img width="600" alt="image" src="https://github.com/user-attachments/assets/7c6e10c0-b680-417f-92e0-2a30cb04226d" /> </p>

A partir de la documentación del fabricante, se identifican los periféricos y GPIOs disponibles. Con esta información, se ha desarrollado un código sencillo que permite controlar un LED mediante GPIO, como introducción a la primera práctica sobre modulación por ancho de pulso (PWM).

Para ello se utilizan funciones clave de ESP-IDF:

- **gpio_reset_pin()** asegura que el pin se encuentra en un estado conocido antes de configurarlo.

- **gpio_set_direction()** establece el pin como salida digital.

- **gpio_set_level()** controla el estado lógico del LED, encendiéndolo o apagándolo.

- **vTaskDelay()** de FreeRTOS genera retardos temporales sin bloquear el sistema, permitiendo un parpadeo periódico del LED.

Este ejemplo sirve como base para comprender el manejo de GPIOs y temporización básica, preparando el entorno para la implementación de señales PWM en prácticas posteriores.

---
## Práctica 1
