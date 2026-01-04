#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

// =======================
// CONFIGURACIÓN PWM
// =======================

#define GPIO_PWM        7
#define PWM_CHANNEL     LEDC_CHANNEL_0
#define PWM_TIMER       LEDC_TIMER_0
#define PWM_RES         LEDC_TIMER_8_BIT
#define PWM_FREQ        5000   // PWM rápido para filtrar (RC)

// Forma de onda del pulso (normalizada 0–1)
float pulso[] = {
    0.0, 0.1, 0.3, 0.6, 1.0,
    0.6, 0.3, 0.15, 0.1,
    0.08, 0.06, 0.05, 0.05, 0.05
};

int pulso_len = sizeof(pulso) / sizeof(float);

// Para 1 Hz → 1000 ms / 14 ≈ 70 ms
#define PULSO_DELAY_MS 70

// =======================
// CONFIGURACIÓN ADC (ESP32-C3)
// =======================

// ADC0 = GPIO0
#define ADC_CANAL ADC_CHANNEL_0

#define UMBRAL_ALTO 2100
#define UMBRAL_BAJO 1800

adc_oneshot_unit_handle_t adc_handle;
static bool pulso_activo = false;

// =======================
// INICIALIZAR PWM
// =======================

void pwm_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution  = PWM_RES,
        .freq_hz          = PWM_FREQ,
        .clk_cfg          = LEDC_APB_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_CHANNEL,
        .timer_sel  = PWM_TIMER,
        .gpio_num   = GPIO_PWM,
        .duty       = 0,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

// =======================
// INICIALIZAR ADC
// =======================

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,     // 0–3.3 V
        .bitwidth = ADC_BITWIDTH_12
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CANAL, &chan_cfg));
}

// =======================
// MAIN
// =======================

void app_main(void)
{
    pwm_init();
    adc_init();

    int max_duty = (1 << PWM_RES) - 1; //255 
    int adc_val;

    printf("PWM + ADC ESP32-C3 iniciado\n");

    while (1) {
        for (int i = 0; i < pulso_len; i++) {

            
            int duty = (int)(pulso[i] * max_duty);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(50)); //estabilizar PWM
            // -------- ADC
            adc_oneshot_read(adc_handle, ADC_CANAL, &adc_val);
            double adc_voltage = adc_val * (3.3 / 4095.0);  // caracterización ADC
            double duty_voltage = (duty / (double)max_duty) * 3.3; // Voltaje PWM
            // -------- Detección de latido
            printf("ADC: %d, Voltaje salida : %.2f V , Voltaje PWM: %.2f V", adc_val, duty_voltage, adc_voltage);

            if (!pulso_activo && adc_val > UMBRAL_ALTO) {
                pulso_activo = true;
                printf("  --> LATIDO");
            }

            if (pulso_activo && adc_val < UMBRAL_BAJO) {
                pulso_activo = false;
            }

            printf("\n");

            vTaskDelay(pdMS_TO_TICKS(PULSO_DELAY_MS));
        }
    }
}
