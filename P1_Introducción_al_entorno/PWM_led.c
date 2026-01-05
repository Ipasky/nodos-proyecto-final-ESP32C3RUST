#include "driver/gpio.h"
#include "esp32c3/rom/ets_sys.h"

#define LED 7

void app_main(void) {
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    while (1) {
        // PWM por software, barrido de brillo
        for (int duty = 0; duty <= 255; duty++) {
            int on_time = (2000 * duty) / 255;  // periodo 2 ms
            int off_time = 2000 - on_time;

            gpio_set_level(LED, 1);
            ets_delay_us(on_time);

            gpio_set_level(LED, 0);
            ets_delay_us(off_time);
        }
    }
}

