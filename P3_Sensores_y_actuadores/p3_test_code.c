#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GPS";

// UART que usaremos para el GPS (mejor no usar UART0 porque es el del monitor)
#define GPS_UART   UART_NUM_1

// Pines (según lo recomendado: GPS TX -> GPIO4, GPS RX -> GPIO5)
#define GPS_RXD    GPIO_NUM_4   // RX de la ESP32 (entra desde TX del GPS)
#define GPS_TXD    GPIO_NUM_5   // TX de la ESP32 (sale hacia RX del GPS)  (opcional si no envías nada)

#define GPS_BAUD   9600

void app_main(void)
{
    // Config UART
    const uart_config_t uart_config = {
        .baud_rate = GPS_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(GPS_UART, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART, GPS_TXD, GPS_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART GPS listo. RX=%d TX=%d BAUD=%d", GPS_RXD, GPS_TXD, GPS_BAUD);
    ESP_LOGI(TAG, "Esperando tramas NMEA... (puede tardar 30-60s en tener FIX)");

    // Buffer para leer bytes y construir líneas
    uint8_t rxbuf[256];
    char line[256];
    int idx = 0;

    while (1) {
        int n = uart_read_bytes(GPS_UART, rxbuf, sizeof(rxbuf), pdMS_TO_TICKS(1000));
        if (n <= 0) continue;

        for (int i = 0; i < n; i++) {
            char c = (char)rxbuf[i];

            // Fin de línea NMEA suele ser \r\n
            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    line[idx] = '\0';
                    // Solo imprime si parece una trama NMEA
                    if (line[0] == '$') {
                        ESP_LOGI(TAG, "%s", line);
                    }
                    idx = 0;
                }
                continue;
            }

            // Acumular caracteres
            if (idx < (int)sizeof(line) - 1) {
                line[idx++] = c;
            } else {
                // Si se llena, resetea la línea
                idx = 0;
            }
        }
    }
}