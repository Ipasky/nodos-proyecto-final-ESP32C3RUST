#include <stdio.h>
#include <stdint.h>

#include "esp32c3/rom/ets_sys.h"
#include "esp_adc/adc_oneshot.h"

// dirección base GPIOs
#define GPIO_BASE       0x60004000
#define GPIO_OUT_W1TS   (GPIO_BASE + 0x08)
#define GPIO_OUT_W1TC   (GPIO_BASE + 0x0C)
#define GPIO_ENABLE_W1TS (GPIO_BASE + 0x20)

#define GPIO_PWM 2

// ADC
#define ADC_CH ADC_CHANNEL_0
adc_oneshot_unit_handle_t adc;

//inicializar GPIO
void gpio_init(int pin)
{
    volatile uint32_t* enable = (uint32_t*) GPIO_ENABLE_W1TS;
    *enable = (1 << pin);
}

// Función para inicializar el ADC
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

// Función para generar el PWM 
void pwm(int pin, float duty, int periodo_us)
{
    int ton  = periodo_us * duty;
    int toff = periodo_us - ton;

    volatile uint32_t* set = (uint32_t*) GPIO_OUT_W1TS;
    volatile uint32_t* clr = (uint32_t*) GPIO_OUT_W1TC;

    *set = (1 << pin);
    ets_delay_us(ton);

    *clr = (1 << pin);
    ets_delay_us(toff);
}

//función en la 
void app_main(void)
{
    //se inicializa el PWM y ADC
    gpio_init(GPIO_PWM);
    adc_init();

    float duty = 0.0; // duty cycle inicial
    int dir = 1;
    int adc_val;

    while (1) {

        // 
        for (int i = 0; i < 50; i++) {
            pwm(GPIO_PWM, duty, 1e4); 
        }

        // Lee el ADC
        adc_oneshot_read(adc, ADC_CH, &adc_val);
        // Se muestra por pantalla
        printf("Duty: %.2f  ADC: %d\n", duty, adc_val);

        // variación del duty cylce para generar un bucle 
        duty += dir * 0.05;
        if (duty >= 1.0) { 
            duty = 1.0; dir = -1;
        }
        if (duty <= 0.0) { 
            duty = 0.0; dir = 1; 
        }
    }
}

