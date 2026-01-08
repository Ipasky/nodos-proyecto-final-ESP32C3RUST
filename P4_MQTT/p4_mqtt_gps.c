#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#include "mqtt_client.h"

static const char *TAG = "GPS+BMP+MQTT";

// ===================== CONFIGURACIÓN WIFI Y MQTT =====================
#define WIFI_SSID      "GokuLeGana"
#define WIFI_PASS      "1234567890"
#define MQTT_BROKER_URI "mqtt://192.168.1.10:1883" // Tu broker MQTT
#define MQTT_TOPIC     "test/gps"

// ===================== GPS (UART) =====================
#define GPS_UART   UART_NUM_1
#define GPS_RXD    GPIO_NUM_20   // ESP RX  <- GPS TX
#define GPS_TXD    GPIO_NUM_21   // ESP TX  -> GPS RX (opcional)
#define GPS_BAUD   9600

// ===================== I2C (BMP/BME280) =====================
#define I2C_PORT        I2C_NUM_0
#define I2C_SDA         GPIO_NUM_8
#define I2C_SCL         GPIO_NUM_10
#define I2C_FREQ_HZ     100000

#define BMP_ADDR_1      0x76
#define BMP_ADDR_2      0x77

#define REG_ID          0xD0
#define REG_CTRL_MEAS   0xF4
#define REG_CONFIG      0xF5
#define REG_PRESS_MSB   0xF7
#define REG_CALIB_00    0x88

#define ID_BMP280       0x58
#define ID_BME280       0x60

static volatile float g_temp_c = 0.0f;
static volatile float g_press_hpa = 0.0f;
static volatile int   g_sensor_ok = 0;

static char g_last_nmea[256] = {0};
static volatile int g_gps_updated = 0;
static portMUX_TYPE g_gps_mux = portMUX_INITIALIZER_UNLOCKED;

// Cliente MQTT
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

// --------------------- BMP/BME calibration ---------------------
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    int32_t  t_fine;
} bmp_calib_t;

static bmp_calib_t cal;
static uint8_t bmp_addr = 0;

// ===================== I2C helpers =====================
static esp_err_t i2c_write_u8(uint8_t dev, uint8_t reg, uint8_t val)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t i2c_read(uint8_t dev, uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev << 1) | I2C_MASTER_READ, true);
    if (len > 1) i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t i2c_probe(uint8_t dev)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t i2c_init_simple(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
        .clk_flags = 0
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &cfg));
    return i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0);
}

// ===================== BMP/BME =====================
static int32_t bmp_comp_temp(int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)cal.dig_T1 << 1))) * ((int32_t)cal.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)cal.dig_T1)) * ((adc_T >> 4) - ((int32_t)cal.dig_T1))) >> 12) *
            ((int32_t)cal.dig_T3)) >> 14;
    cal.t_fine = var1 + var2;
    return (cal.t_fine * 5 + 128) >> 8; // 0.01 ºC
}

static uint32_t bmp_comp_press(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)cal.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)cal.dig_P6;
    var2 = var2 + ((var1 * (int64_t)cal.dig_P5) << 17);
    var2 = var2 + (((int64_t)cal.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)cal.dig_P3) >> 8) + ((var1 * (int64_t)cal.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)cal.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)cal.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)cal.dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)cal.dig_P7) << 4);
    return (uint32_t)p; // Pa*256 (Q24.8)
}

// ===================== BMP/BME init/read =====================
static esp_err_t bmp_read_calib(uint8_t dev)
{
    uint8_t b[24];
    esp_err_t err = i2c_read(dev, REG_CALIB_00, b, sizeof(b));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "calib read failed: %s", esp_err_to_name(err));
        return err;
    }

    cal.dig_T1 = (uint16_t)(b[1] << 8 | b[0]);
    cal.dig_T2 = (int16_t)(b[3] << 8 | b[2]);
    cal.dig_T3 = (int16_t)(b[5] << 8 | b[4]);

    cal.dig_P1 = (uint16_t)(b[7] << 8 | b[6]);
    cal.dig_P2 = (int16_t)(b[9] << 8 | b[8]);
    cal.dig_P3 = (int16_t)(b[11] << 8 | b[10]);
    cal.dig_P4 = (int16_t)(b[13] << 8 | b[12]);
    cal.dig_P5 = (int16_t)(b[15] << 8 | b[14]);
    cal.dig_P6 = (int16_t)(b[17] << 8 | b[16]);
    cal.dig_P7 = (int16_t)(b[19] << 8 | b[18]);
    cal.dig_P8 = (int16_t)(b[21] << 8 | b[20]);
    cal.dig_P9 = (int16_t)(b[23] << 8 | b[22]);

    return ESP_OK;
}

static esp_err_t bmp_init_detect(void)
{
    uint8_t candidates[2] = {BMP_ADDR_1, BMP_ADDR_2};

    for (int i = 0; i < 2; i++) {
        uint8_t addr = candidates[i];
        if (i2c_probe(addr) != ESP_OK) continue;

        uint8_t id = 0;
        if (i2c_read(addr, REG_ID, &id, 1) != ESP_OK) continue;

        if (id == ID_BMP280 || id == ID_BME280) {
            bmp_addr = addr;
            ESP_LOGI(TAG, "Sensor encontrado en 0x%02X, ID=0x%02X", bmp_addr, id);

            esp_err_t err = bmp_read_calib(bmp_addr);
            if (err != ESP_OK) return err;

            // osrs_t=x1, osrs_p=x1, normal mode => 0x27
            err = i2c_write_u8(bmp_addr, REG_CTRL_MEAS, 0x27);
            if (err != ESP_OK) return err;

            err = i2c_write_u8(bmp_addr, REG_CONFIG, 0x00);
            if (err != ESP_OK) return err;

            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t bmp_read_tp(float *temp_c, float *press_hpa)
{
    uint8_t d[6];
    esp_err_t err = i2c_read(bmp_addr, REG_PRESS_MSB, d, sizeof(d));
    if (err != ESP_OK) return err;

    int32_t adc_P = (int32_t)((d[0] << 12) | (d[1] << 4) | (d[2] >> 4));
    int32_t adc_T = (int32_t)((d[3] << 12) | (d[4] << 4) | (d[5] >> 4));

    int32_t t100 = bmp_comp_temp(adc_T);   // 0.01°C
    uint32_t p256 = bmp_comp_press(adc_P); // Pa*256

    double pa = (double)p256 / 256.0;
    *temp_c = (float)(t100 / 100.0);
    *press_hpa = (float)(pa / 100.0);
    return ESP_OK;
}

// ===================== WIFI & MQTT Event Handlers =====================
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Wi-Fi connected. IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatching base=%s, event_id=%" PRId32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected");
        mqtt_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT Disconnected");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT Error");
        break;
    default:
        break;
    }
}

static void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// ===================== TASKS =====================
static void bmp_task(void *arg)
{
    if (i2c_init_simple() != ESP_OK) {
        ESP_LOGE(TAG, "Error init I2C");
        vTaskDelete(NULL);
    }

    if (bmp_init_detect() != ESP_OK) {
        ESP_LOGE(TAG, "No se encontró BMP/BME280 en 0x76/0x77");
        g_sensor_ok = 0;
        vTaskDelete(NULL);
    }

    g_sensor_ok = 1;

    while (1) {
        float t, p;
        if (bmp_read_tp(&t, &p) == ESP_OK) {
            g_temp_c = t;
            g_press_hpa = p;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Tarea GPS: lee UART y guarda última línea NMEA
static void gps_uart_task(void *arg)
{
    uint8_t rxbuf[256];
    char line[256];
    int idx = 0;

    while (1) {
        int n = uart_read_bytes(GPS_UART, rxbuf, sizeof(rxbuf), pdMS_TO_TICKS(200));
        if (n <= 0) continue;

        for (int i = 0; i < n; i++) {
            char c = (char)rxbuf[i];

            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    line[idx] = '\0';
                    idx = 0;

                    if (line[0] == '$') {
                        portENTER_CRITICAL(&g_gps_mux);
                        strncpy(g_last_nmea, line, sizeof(g_last_nmea) - 1);
                        g_last_nmea[sizeof(g_last_nmea) - 1] = '\0';
                        g_gps_updated = 1;
                        portEXIT_CRITICAL(&g_gps_mux);
                    }
                }
                continue;
            }

            if (idx < (int)sizeof(line) - 1) {
                line[idx++] = c;
            } else {
                idx = 0;
            }
        }
    }
}

// Tarea: Publica en MQTT cada 2 segundos
static void publisher_task(void *arg)
{
    char local_nmea[256];
    char payload[512];

    while (1) {
        if (!mqtt_connected) {
             vTaskDelay(pdMS_TO_TICKS(2000));
             continue;
        }

        int had_new = 0;

        portENTER_CRITICAL(&g_gps_mux);
        had_new = g_gps_updated;
        if (had_new) {
            strncpy(local_nmea, g_last_nmea, sizeof(local_nmea) - 1);
            local_nmea[sizeof(local_nmea) - 1] = '\0';
        } else {
             strcpy(local_nmea, "-");
        }
        g_gps_updated = 0; // reset
        portEXIT_CRITICAL(&g_gps_mux);

        // Construir JSON
        if (g_sensor_ok) {
            snprintf(payload, sizeof(payload), 
                "{\"temp\": %.2f, \"press\": %.2f, \"gps\": \"%s\"}",
                g_temp_c, g_press_hpa, local_nmea);
        } else {
             snprintf(payload, sizeof(payload), 
                "{\"temp\": null, \"press\": null, \"gps\": \"%s\"}",
                local_nmea);
        }

        // Publicar
        int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 1, 0);
        ESP_LOGI(TAG, "Sent publish successful, msg_id=%d, payload=%s", msg_id, payload);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    // NVS Init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // WiFi Init
    wifi_init_sta();

    // MQTT Init
    mqtt_app_start();

    // UART GPS
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
    ESP_LOGI(TAG, "I2C SDA=%d SCL=%d (BMP/BME 0x76/0x77)", I2C_SDA, I2C_SCL);

    xTaskCreate(bmp_task,       "bmp_task",       4096, NULL, 5, NULL);
    xTaskCreate(gps_uart_task,  "gps_uart_task",  4096, NULL, 6, NULL);
    xTaskCreate(publisher_task, "publisher_task", 4096, NULL, 4, NULL);
}