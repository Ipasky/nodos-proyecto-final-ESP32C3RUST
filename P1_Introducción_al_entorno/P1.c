#include <stdint.h>
#include "esp32c3/rom/ets_sys.h"

#define GPIO_BASE       0x60004000
#define GPIO_OUT_W1TS   (GPIO_BASE + 0x08)
#define GPIO_OUT_W1TC   (GPIO_BASE + 0x0C)
#define GPIO_ENABLE_W1TS (GPIO_BASE + 0x20)
#define GPIO_ENABLE_W1TC (GPIO_BASE + 0x24)

#define LED 7
#define DELAY_MS 500

void app_main(void) {
    volatile uint32_t* gpio_out_w1ts = (volatile uint32_t*) GPIO_OUT_W1TS;
    volatile uint32_t* gpio_out_w1tc = (volatile uint32_t*) GPIO_OUT_W1TC;
    volatile uint32_t* gpio_enable_w1ts = (volatile uint32_t*) GPIO_ENABLE_W1TS;

    // Configura GPIO7 como salida
    *gpio_enable_w1ts = (1 << LED);

    while(1) {
        // Encender LED
        *gpio_out_w1ts = (1 << LED);
        ets_delay_us(DELAY_MS * 1000);

        // Apagar LED
        *gpio_out_w1tc = (1 << LED);
        ets_delay_us(DELAY_MS * 1000);
    }
}
