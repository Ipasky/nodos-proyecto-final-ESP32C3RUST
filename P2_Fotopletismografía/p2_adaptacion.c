#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define GPIO_PWM 7
#define PWM_CHANNEL 0
#define PWM_TIMER 0
#define PWM_RES LEDC_TIMER_6_BIT
#define PWM_FREQ 20000  // PWM rápido para filtrado

float pulso[] = {
    0.0, 0.1, 0.3, 0.6, 1.0, 0.6, 0.3, 0.15, 0.1,
    0.08, 0.06, 0.05, 0.05, 0.05
};

int pulso_len = sizeof(pulso) / sizeof(float);

void pwm_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,  // cambio recomendado
        .timer_num = PWM_TIMER,
        .duty_resolution = PWM_RES,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_APB_CLK
    };

    ledc_channel_config_t channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,  // cambio recomendado
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .gpio_num = GPIO_PWM,
        .duty = 0,
        .hpoint = 0
    };

    if (ledc_timer_config(&timer) != ESP_OK) {
        printf("Error al configurar timer PWM\n");
        return;
    }
    if (ledc_channel_config(&channel) != ESP_OK) {
        printf("Error al configurar canal PWM\n");
        return;
    }
}

void app_main(void) {
    pwm_init();

    int max_duty = (1 << PWM_RES) - 1;

    while (1) {
        for (int i = 0; i < pulso_len; i++) {
            int duty = (int)(pulso[i] * max_duty);  // cast explícito
            ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);

            vTaskDelay(pdMS_TO_TICKS(20)); // controla BPM
            printf();
        }
    }
}

