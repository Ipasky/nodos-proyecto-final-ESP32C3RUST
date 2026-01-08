#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GPS_STEPPER";

// GPS (UART)
#define GPS_UART_NUM        UART_NUM_1
#define GPS_TX_PIN          21  // ESP32 TX (GPS RX)
#define GPS_RX_PIN          20  // ESP32 RX (GPS TX)
#define GPS_BUF_SIZE        1024

// STEPPER MOTOR (ULN2003 / 28BYJ-48)
// Pines de control
#define STEP_PIN_1          6
#define STEP_PIN_2          7
#define STEP_PIN_3          8
#define STEP_PIN_4          9
#define GPIO_STEPPER_MASK   ((1ULL<<STEP_PIN_1) | (1ULL<<STEP_PIN_2) | (1ULL<<STEP_PIN_3) | (1ULL<<STEP_PIN_4))

// Características del motor
// El 28BYJ-48 tiene aprox 2038 pasos por vuelta en modo half-step (8 pasos)
#define STEPS_PER_REV       2038 

// Variable global para comunicar el rumbo (protegida por simplicidad, idealmente usaríamos colas/mutex)
volatile float g_target_heading = 0.0f;
volatile int   g_current_stepper_pos = 0; // Posición actual en pasos (0 a STEPS_PER_REV)


// Secuencia de 8 pasos (Half-step) para mayor suavidad
const int step_sequence[8][4] = {
    {1, 0, 0, 1}, 
    {1, 0, 0, 0}, 
    {1, 1, 0, 0}, 
    {0, 1, 0, 0}, 
    {0, 1, 1, 0}, 
    {0, 0, 1, 0}, 
    {0, 0, 1, 1}, 
    {0, 0, 0, 1}  
};

void stepper_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = GPIO_STEPPER_MASK,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Stepper inicializado en pines %d, %d, %d, %d", STEP_PIN_1, STEP_PIN_2, STEP_PIN_3, STEP_PIN_4);
}

// Mueve el motor un solo paso en la dirección indicada
void stepper_step_once(int direction) {
    static int step_index = 0;
    
    if (direction > 0) {
        step_index++;
        if (step_index > 7) step_index = 0;
    } else {
        step_index--;
        if (step_index < 0) step_index = 7;
    }

    gpio_set_level(STEP_PIN_1, step_sequence[step_index][0]);
    gpio_set_level(STEP_PIN_2, step_sequence[step_index][1]);
    gpio_set_level(STEP_PIN_3, step_sequence[step_index][2]);
    gpio_set_level(STEP_PIN_4, step_sequence[step_index][3]);

    esp_rom_delay_us(1000); // 1ms entre pasos
}


void stepper_task(void *arg) {
    while (1) {
        // 1. Convertir el rumbo objetivo (grados) a pasos (0..2038)
        int target_step = (int)((g_target_heading / 360.0f) * STEPS_PER_REV);

        if (target_step >= STEPS_PER_REV) target_step = STEPS_PER_REV - 1;
        
        int diff = target_step - g_current_stepper_pos;

        if (abs(diff) > 5) {
            if (diff > 0) {
                stepper_step_once(1);
                g_current_stepper_pos++;
            } else {
                stepper_step_once(-1);
                g_current_stepper_pos--;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        
        if (abs(diff) < 5) vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// LÓGICA DEL GPS
char* get_nmea_field(char *line, int index) {
    static char buffer[32];
    char *p = line;
    int current_idx = 0;
    while(current_idx < index && p) {
        p = strchr(p, ',');
        if(p) {
            p++;
            current_idx++;
        }
    }

    if (!p) return NULL;

    int i = 0;
    while(p[i] != ',' && p[i] != '*' && p[i] != '\0' && p[i] != '\r' && i < 31) {
        buffer[i] = p[i];
        i++;
    }
    buffer[i] = '\0';
    return buffer;
}

void gps_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(GPS_UART_NUM, &uart_config);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uint8_t *data = (uint8_t *) malloc(GPS_BUF_SIZE);
    char line_buffer[256];
    int line_idx = 0;

    ESP_LOGI(TAG, "Esperando datos GPS...");

    while (1) {
        // Leer bytes de la UART
        int len = uart_read_bytes(GPS_UART_NUM, data, GPS_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = (char)data[i];
                
                // Acumular linea hasta encontrar salto de línea
                if (c == '\n') {
                    line_buffer[line_idx] = 0;
                    
                    // Buscar trama $GNRMC o $GPRMC
                    if (strstr(line_buffer, "RMC")) {
                        // Campos RMC: 
                        // 0:ID, 1:Time, 2:Status(A/V), 3:Lat, 4:N/S, 5:Lon, 6:E/W, 7:Speed, 8:Course
                        
                        char *status = get_nmea_field(line_buffer, 2);
                        char *course = get_nmea_field(line_buffer, 8);

                        if (status && status[0] == 'A') {
                            if (course && strlen(course) > 0) {
                                float heading = atof(course);
                                ESP_LOGI(TAG, "Rumbo: %.2f deg", heading);
                                g_target_heading = heading;
                            }
                        } else {
                             // ESP_LOGW(TAG, "Esperando FIX...");
                        }
                    }

                    line_idx = 0;
                } else if (c != '\r') {
                    if (line_idx < 255) {
                        line_buffer[line_idx++] = c;
                    }
                }
            }
        }
    }
}


void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Practica 3 Adaptada (GPS + Stepper)");

    stepper_init();

    xTaskCreate(stepper_task, "stepper_task", 2048, NULL, 5, NULL);
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
}