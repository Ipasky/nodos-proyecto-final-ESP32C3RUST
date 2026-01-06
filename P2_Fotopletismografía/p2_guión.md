# Guión Practica 2 Captura y visualización de una fotopletismografía
## Adaptación de la Práctica 2 a la plataforma ESP32-C3

En esta práctica se ha trabajado la captura y visualización de una señal de fotopletismografía, cuyo objetivo principal es familiarizarse con los conceptos fundamentales de un sistema de adquisición de datos: conversión analógico-digital, muestreo temporal, almacenamiento de muestras y posterior visualización y análisis de la señal.

La práctica original estaba diseñada para realizarse sobre una Raspberry Pi, utilizando un ADC externo (MCP3008) y un DAC externo (MCP4911) comunicados mediante el bus SPI. El entorno de desarrollo se basaba en la librería WiringPi y en el uso de periféricos externos para la conversión analógica y digital.

Sin embargo, para la realización de esta práctica se ha decidido adaptar su contenido a la plataforma ESP32-C3, lo que ha requerido una modificación tanto del hardware empleado como del entorno software, intentando mantener los objetivos originales.

## Cambios en la plataforma hardware

La ESP32-C3 es un microcontrolador que integra un conversor analógico-digital (ADC) interno, por lo que no es necesario el uso de un ADC externo como en la práctica original. El ADC interno de la ESP32-C3 dispone de una resolución de 12 bits, proporcionando valores entre 0 y 4095 para un rango de entrada de 0 a 3.3 V.

Por otro lado, a diferencia de otros modelos de ESP32, la ESP32-C3 no dispone de conversor digital-analógico (DAC). Esta parte de la práctica original de la generación de señales mediante DAC y a la caracterización del ADC a partir de un DAC externo no puede llevarse a cabo y se ha cambiado su forma de realización.

Por estos motivos, la práctica se centra en:

- El uso del ADC interno de la ESP32-C3.

- La adquisición de señales analógicas reales.

- El análisis de los valores obtenidos a partir de diferentes niveles de tensión.
## Cambios en el entorno software

Mientras que la práctica original utilizaba una Raspberry Pi con WiringPi, en esta adaptación se emplea el entorno ESP-IDF, que es el framework oficial de Espressif para el desarrollo en ESP32.

Para la lectura del ADC se utiliza el modo adc_oneshot, que permite realizar conversiones puntuales bajo demanda, facilitando el control del instante de muestreo. 
Dado que no se dispone de los elementos necesarios para la realización óptima de la práctica original, estos se han sustituido por otros planteamientos que permiten mantener los objetivos didácticos de la misma. En primer lugar, para sustituir la pinza encargada de capturar el pulso cardíaco, se ha generado una señal periódica mediante modulación por ancho de pulso (PWM), tal y como se realizó en la práctica anterior. De esta forma, se consigue simular una señal de entrada analógica similar, aunque no ideal, a la que proporcionaría la pinza de fotopletismografía.

Esta señal PWM representa una aproximación al comportamiento de un pulso cardíaco real, permitiendo trabajar con una señal variable en el tiempo cuya amplitud media puede ser interpretada como una señal analógica tras su filtrado.


Además, se plantea la implementación de un filtro hardware paso bajo, para reducir el posible ruido presente en las medidas y obtener una señal más suave y representativa. Para ello, es necesario tener en cuenta que la frecuencia cardíaca es baja, del orden de 1 Hz, por lo que el filtro diseñado debe permitir el paso de las componentes principales de la señal y atenuar aquellas de frecuencia superior, especialmente las asociadas a la señal PWM.
Este filtro se implementa mediante una red **RC**, formada por una resistencia de **10 kΩ** y un condensador de **1 µF**, cuya función es atenuar las componentes de alta frecuencia de la señal y dejar pasar aquellas de baja frecuencia.

La frecuencia de corte del filtro RC viene determinada por la expresión:

$$
f_c = \frac{1}{2\pi RC}
$$

Sustituyendo los valores seleccionados para el diseño del filtro:

$$
R = 10\,000\ \Omega
$$

$$
C = 1 \times 10^{-6}\ \text{F}
$$

se obtiene:

$$
f_c = \frac{1}{2\pi \cdot 10\,000 \cdot 1 \times 10^{-6}} \approx 15.9\ \text{Hz}
$$

Este valor de frecuencia de corte es suficientemente superior a la frecuencia cardíaca, que se encuentra en torno a **1 Hz**, permitiendo el paso de las componentes principales de la señal asociadas al pulso cardíaco. Al mismo tiempo, el filtro atenúa de forma significativa las componentes de mayor frecuencia, especialmente las generadas por la señal PWM utilizada para simular el pulso.


Por último, se hace uso del ADC interno de la ESP32-C3, el cual permite leer el valor de tensión presente en el pin seleccionado para su posterior procesamiento. Este ADC proporciona una resolución de 12 bits, lo que permite obtener valores digitales comprendidos entre 0 y 4095, proporcionales al voltaje de entrada.
## Configuración del ADC en la ESP32-C3

La ESP32-C3 integra un conversor analógico-digital de tipo **SAR (Successive Approximation Register)**. Este ADC está controlado mediante un conjunto de registros accesibles desde el bus APB, que permiten configurar su funcionamiento interno, gestionar el muestreo y leer los datos convertidos.

El acceso directo a estos registros proporciona un control total sobre el ADC, pero también requiere un conocimiento detallado de la arquitectura interna del periférico y de las secuencias de configuración necesarias.

---

## Registros principales del periférico SAR ADC

### APB_SARADC_APB_ADC_ARB_CTRL_REG
Este registro controla el **árbitro del ADC**, encargado de gestionar el acceso cuando existen múltiples fuentes que desean utilizar el ADC de forma simultánea (CPU, DMA u otros periféricos internos). Su función principal es evitar conflictos y priorizar el acceso a los distintos recursos del ADC.

---

### APB_SARADC_FILTER_CTRL0_REG
Permite configurar **filtros digitales internos** del ADC, que pueden utilizarse para reducir el ruido de alta frecuencia o suavizar las lecturas mediante promediado interno. En esta práctica, el filtrado se realiza externamente mediante un filtro hardware RC y por software, por lo que este registro no resulta imprescindible.

---

### APB_SARADC_1_DATA_STATUS_REG  
### APB_SARADC_2_DATA_STATUS_REG
Son registros de **solo lectura** que contienen el resultado de la conversión realizada por el ADC.  
- `ADC1_DATA_STATUS` corresponde al ADC1  
- `ADC2_DATA_STATUS` corresponde al ADC2  

En estos registros se obtiene finalmente el valor digital proporcional al voltaje de entrada.

---

### APB_SARADC_THRES0_CTRL_REG  
### APB_SARADC_THRES1_CTRL_REG  
### APB_SARADC_THRES_CTRL_REG
Estos registros permiten definir **umbrales de comparación** para el ADC. Se utilizan para generar interrupciones cuando la señal analógica supera determinados valores, facilitando la detección de eventos. Este mecanismo no es necesario para la lectura periódica de muestras realizada en esta práctica.

---

### APB_SARADC_INT_ENA_REG
Registro encargado de habilitar las **interrupciones del ADC**, como la finalización de una conversión o el cruce de umbrales configurados previamente.

---

### APB_SARADC_INT_RAW_REG
Registro que indica el **estado bruto de las interrupciones**, antes de aplicar cualquier tipo de máscara o filtrado.

---

### APB_SARADC_INT_ST_REG
Muestra el **estado final de las interrupciones**, una vez procesadas, permitiendo identificar el evento que ha generado la interrupción.

---

### APB_SARADC_INT_CLR_REG
Registro de escritura utilizado para **limpiar las interrupciones** del ADC tras haber sido atendidas.

---

### APB_SARADC_DMA_CONF_REG
Permite configurar el uso de **DMA** para el ADC, facilitando la transferencia de grandes volúmenes de datos a memoria sin intervención directa de la CPU. Este modo está orientado a aplicaciones de alta velocidad y no se emplea en esta práctica.

---

### APB_SARADC_APB_ADC_CLKM_CONF_REG
Controla el **reloj del ADC**, definiendo la frecuencia de funcionamiento del conversor y sus divisores de reloj. Una configuración incorrecta de este registro puede provocar errores en las conversiones.

---

### APB_SARADC_APB_TSENS_CTRL_REG  
### APB_SARADC_APB_TSENS_CTRL2_REG
Registros asociados al **sensor de temperatura interno**, que comparte parte de la infraestructura del ADC. No se utilizan en el desarrollo de esta práctica.

---

### APB_SARADC_CALI_REG
Registro de **calibración del ADC**, utilizado para ajustar parámetros como el offset y la ganancia. Su correcta configuración es clave para obtener medidas precisas, aunque requiere un conocimiento avanzado del hardware.

---

### APB_SARADC_APB_CTRL_DATE_REG
Registro de control de versión del periférico, empleado para identificar la revisión del hardware y garantizar compatibilidad.

---

## Justificación del uso de `adc_oneshot`

Aunque el ADC interno de la ESP32-C3 puede configurarse mediante acceso directo a los registros del periférico SAR ADC, esta aproximación requiere un conocimiento detallado de la arquitectura interna del conversor, así como de las secuencias de inicialización, temporización y calibración.

Por este motivo, y con el objetivo de centrar la práctica en los conceptos fundamentales de adquisición y procesado de señales, se ha optado por utilizar el driver `adc_oneshot`, que abstrae la complejidad del hardware manteniendo un control preciso sobre el instante de muestreo. De este modo, se reserva la configuración del ADC para las siguientes prácticas centradas en la adquisición de datos. 

Para la lectura de la señal analógica se ha utilizado el **driver `adc_oneshot`** proporcionado por ESP-IDF, el cual permite realizar conversiones puntuales del ADC bajo demanda. Este enfoque facilita el control del instante de muestreo y resulta adecuado para aplicaciones de adquisición de señales de baja frecuencia, como la tratada en esta práctica.

La inicialización del ADC se realiza mediante la siguiente función:

```c
void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init, &adc);

    adc_oneshot_chan_cfg_t cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12
    };
    adc_oneshot_config_channel(adc, ADC_CH, &cfg);
}
```
En primer lugar, se selecciona la unidad ADC a utilizar mediante el campo unit_id. En la ESP32-C3 se emplea el ADC1, que es el encargado de realizar las conversiones analógicas bajo control de la CPU. Esta selección es equivalente, a bajo nivel, a configurar el árbitro del ADC y habilitar la unidad correspondiente mediante los registros del periférico SAR ADC.

A continuación, se configura el canal ADC concreto a través de la estructura **adc_oneshot_chan_cfg_t** . En esta configuración se definen dos parámetros fundamentales:

- Atenuación (atten): se selecciona ADC_ATTEN_DB_11, lo que permite medir tensiones de hasta aproximadamente 3.3 V. A nivel hardware, esta configuración ajusta la red de atenuación interna del ADC, equivalente a la configuración manual de los registros de ganancia y rango de entrada.

- Resolución (bitwidth): se establece una resolución de 12 bits, por lo que el valor digital obtenido estará comprendido entre 0 y 4095. Este parámetro corresponde directamente al número de bits efectivos del conversor SAR.

Finalmente, la llamada a **adc_oneshot_config_channel()** asocia el canal ADC seleccionado con la configuración definida, dejando el ADC preparado para realizar conversiones puntuales cuando se soliciten mediante la función de lectura.

En conjunto, aunque el uso de adc_oneshot no refleja el acceso directo a los registros del periférico SAR ADC, el proceso realizado es conceptualmente equivalente a una configuración manual del ADC: se selecciona el bloque, se configura el canal de entrada, se ajusta el rango de tensión y se define la resolución del conversor. De este modo, se mantiene el control sobre los parámetros fundamentales del sistema de adquisición, evitando al mismo tiempo la complejidad asociada a la configuración directa de los registros de bajo nivel. 

---
## Análisis de los resultados

De este modo, se cargó el código en el dispositivo y se extrajeron los datos capturados por el ADC. 
<p align="center">
<img width="200" alt="image" src="https://github.com/user-attachments/assets/f3cc2b03-b68d-46f9-9d77-d215f1d0655b" />
</p>
<p align="center">
<img width="600"  alt="image" src="https://github.com/user-attachments/assets/d4318ef5-aaec-4fb3-9cc2-760e86bbcaf4" />
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/7de17706-15e0-4840-8149-c01940a21695"
       width="600" style="margin-bottom: 20px;" />

  </p>
<p align="center">
  <img src="https://github.com/user-attachments/assets/ea37f165-f1f2-4600-9f7b-ab97c8d630cc"
       width="600" style="margin-bottom: 20px;" />

  </p>
<p align="center">
  <img src="https://github.com/user-attachments/assets/9c9cbdb9-12e5-40f5-adf0-f33c53f572fb"
       width="600" />
</p>




| Parámetro    | Valor (LSB) | Valor (V) |
|:------------:|:-----------:|:---------:|
| Offset error | 21          | 16.9 mV   |
| Gain error   | 0.5         | 0.40 mV   |
| DNL máximo   | 31          | 25 mV     |


