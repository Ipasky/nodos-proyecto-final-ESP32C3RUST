#include <stdint.h>
#include "esp32c3/rom/ets_sys.h"

#define GPIO_BASE        0x60004000
#define GPIO_OUT_W1TS    (GPIO_BASE + 0x08)
#define GPIO_OUT_W1TC    (GPIO_BASE + 0x0C)
#define GPIO_ENABLE_W1TS (GPIO_BASE + 0x20)

#define LED_PIN 2       
#define PWM_RES 256     // resolución 8 bits
#define PWM_DELAY_US 50 // microsegundos por paso 

// registros GPIO
volatile uint32_t* gpio_out_w1ts = (volatile uint32_t*) GPIO_OUT_W1TS;
volatile uint32_t* gpio_out_w1tc = (volatile uint32_t*) GPIO_OUT_W1TC;
volatile uint32_t* gpio_enable_w1ts = (volatile uint32_t*) GPIO_ENABLE_W1TS;

void app_main(void) {
    // Configurar LED como salida
    *gpio_enable_w1ts = (1 << LED_PIN);

    while (1) {
        //sube brillo
        for (int duty = 0; duty < PWM_RES; duty++) {
            for (int i = 0; i < PWM_RES; i++) {
                if (i < duty)
                    *gpio_out_w1ts = (1 << LED_PIN); // LED encendido
                else
                    *gpio_out_w1tc = (1 << LED_PIN); // LED apagado
                ets_delay_us(PWM_DELAY_US);
            }
        }

        // baja brillo
        for (int duty = PWM_RES - 1; duty >= 0; duty--) {
            for (int i = 0; i < PWM_RES; i++) {
                if (i < duty)
                    *gpio_out_w1ts = (1 << LED_PIN);
                else
                    *gpio_out_w1tc = (1 << LED_PIN);
                ets_delay_us(PWM_DELAY_US);
            }
        }
    }
}
