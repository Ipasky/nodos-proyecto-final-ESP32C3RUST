#include <stdio.h>
#include "driver/gpio.h"
#include "esp32c3/rom/ets_sys.h"  // Para ets_delay_us()

#define LED GPIO_NUM_7

void app_main(void)
{
    // Inicializamos el pin como salida
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    while (1)
    {
        // LED apagado
        gpio_set_level(LED, 0);

        // Delay de 1 segundo (1000000 microsegundos)
        ets_delay_us(1000000);

        // LED encendido
        gpio_set_level(LED, 1);

        // Delay de 1 segundo
        ets_delay_us(1000000);
    }
}


