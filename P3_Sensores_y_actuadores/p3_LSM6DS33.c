#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "driver/i2c.h"
#include "driver/ledc.h" 

static const char *TAG = "P3_FINAL";

#define SERVO_GPIO          7   
#define SERVO_FREQ_HZ       50  
#define SERVO_PERIOD_US     20000 
#define LEDC_RES            LEDC_TIMER_14_BIT 

#define PULSE_STOP          1500
#define PULSE_CW            1300  
#define PULSE_CCW           1700  

#define I2C_PORT            I2C_NUM_0
#define I2C_SDA             GPIO_NUM_8
#define I2C_SCL             GPIO_NUM_9
#define I2C_FREQ_HZ         400000
#define I2C_TIMEOUT_MS      100

// Direcciones y Registros LSM6DS33
#define LSM6_ADDR_6A        0x6A
#define LSM6_ADDR_6B        0x6B
#define REG_WHO_AM_I        0x0F
#define WHO_AM_I_EXPECTED   0x69
#define REG_CTRL1_XL        0x10
#define REG_CTRL2_G         0x11
#define REG_CTRL3_C         0x12
#define REG_OUT_TEMP_L      0x20

// Configuración de Filtros
#define GYRO_DEADZONE       1.0f 
const float accel_g_per_lsb = 0.061f / 1000.0f;
const float gyro_dps_per_lsb = 8.75f / 1000.0f;

float offset_ax = 0, offset_ay = 0, offset_az = 0;
float offset_gx = 0, offset_gy = 0, offset_gz = 0;

// Funciones I2C
static esp_err_t i2c_write_u8(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return i2c_master_write_to_device(I2C_PORT, addr, buf, sizeof(buf), pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static esp_err_t i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_PORT, addr, &reg, 1, data, len, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static uint8_t detect_lsm6_addr(void) {
    uint8_t who = 0x00;
    if (i2c_read(LSM6_ADDR_6B, REG_WHO_AM_I, &who, 1) == ESP_OK && who == WHO_AM_I_EXPECTED) return LSM6_ADDR_6B;
    if (i2c_read(LSM6_ADDR_6A, REG_WHO_AM_I, &who, 1) == ESP_OK && who == WHO_AM_I_EXPECTED) return LSM6_ADDR_6A;
    return 0;
}

static esp_err_t lsm6_init(uint8_t addr) {
    ESP_ERROR_CHECK(i2c_write_u8(addr, REG_CTRL3_C, 0x44));   
    ESP_ERROR_CHECK(i2c_write_u8(addr, REG_CTRL1_XL, 0x40));  
    ESP_ERROR_CHECK(i2c_write_u8(addr, REG_CTRL2_G, 0x40));   
    return ESP_OK;
}

static inline int16_t le16(const uint8_t *p) {
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

// CALIBRACIÓN INICIAL
void calibrate_sensor(uint8_t addr) {
    const int num_samples = 200;
    float sum_ax = 0, sum_ay = 0, sum_az = 0;
    float sum_gx = 0, sum_gy = 0, sum_gz = 0;
    uint8_t b[14];

    printf("\nINICIANDO CALIBRACION (5s) - NO TOCAR EL SENSOR\n");
    for (int i = 5; i > 0; i--) {
        printf("%d...\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("Leyendo muestras...\n");

    for (int i = 0; i < num_samples; i++) {
        if (i2c_read(addr, REG_OUT_TEMP_L, b, sizeof(b)) == ESP_OK) {
            int16_t raw_gx = le16(&b[2]);
            int16_t raw_gy = le16(&b[4]);
            int16_t raw_gz = le16(&b[6]);
            int16_t raw_ax = le16(&b[8]);
            int16_t raw_ay = le16(&b[10]);
            int16_t raw_az = le16(&b[12]);

            sum_ax += (raw_ax * accel_g_per_lsb);
            sum_ay += (raw_ay * accel_g_per_lsb);
            sum_az += (raw_az * accel_g_per_lsb);
            sum_gx += (raw_gx * gyro_dps_per_lsb);
            sum_gy += (raw_gy * gyro_dps_per_lsb);
            sum_gz += (raw_gz * gyro_dps_per_lsb);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    offset_ax = sum_ax / num_samples;
    offset_ay = sum_ay / num_samples;
    offset_az = (sum_az / num_samples) - 1.0f;
    offset_gx = sum_gx / num_samples;
    offset_gy = sum_gy / num_samples;
    offset_gz = sum_gz / num_samples;

    printf("CALIBRACION COMPLETA.\n\n");
}


static void servo_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_RES,
        .freq_hz          = SERVO_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num   = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
}

static void servo_write_us(uint32_t pulse_us) {
    uint32_t max_duty = (1 << 14) - 1; 
    uint32_t duty = (uint32_t)((pulse_us * (uint64_t)max_duty) / SERVO_PERIOD_US);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void servo_stop_hard(void) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}


void app_main(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
        .clk_flags = 0,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0));
    
    uint8_t addr = detect_lsm6_addr();
    if (addr == 0) return;
    ESP_ERROR_CHECK(lsm6_init(addr));

    servo_init();

    calibrate_sensor(addr);

    float pitch = 0, roll = 0, yaw = 0; 
    float pitch_acc_f = 0, roll_acc_f = 0;
    const float alpha_lp = 0.10f;
    const float beta_cf  = 0.98f;
    int64_t t_prev = esp_timer_get_time();

    while (1) {
        uint8_t b[14];
        if (i2c_read(addr, REG_OUT_TEMP_L, b, sizeof(b)) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Raw data
        int16_t raw_temp = le16(&b[0]);
        int16_t raw_gx = le16(&b[2]);
        int16_t raw_gy = le16(&b[4]);
        int16_t raw_gz = le16(&b[6]);
        int16_t raw_ax = le16(&b[8]);
        int16_t raw_ay = le16(&b[10]);
        int16_t raw_az = le16(&b[12]);

        // Conversión con Offset
        float ax = (raw_ax * accel_g_per_lsb) - offset_ax;
        float ay = (raw_ay * accel_g_per_lsb) - offset_ay;
        float az = (raw_az * accel_g_per_lsb) - offset_az;
        
        float gx = (raw_gx * gyro_dps_per_lsb) - offset_gx;
        float gy = (raw_gy * gyro_dps_per_lsb) - offset_gy;
        float gz = (raw_gz * gyro_dps_per_lsb) - offset_gz;

        // Temperatura
        float temp_c = 25.0f + (raw_temp / 16.0f);

        // Deadzone para el Drift
        if (fabs(gx) < GYRO_DEADZONE) gx = 0;
        if (fabs(gy) < GYRO_DEADZONE) gy = 0;
        if (fabs(gz) < GYRO_DEADZONE) gz = 0;

        // Tiempo delta
        int64_t t_now = esp_timer_get_time();
        float dt = (float)(t_now - t_prev) / 1e6f;
        t_prev = t_now;
        if (dt <= 0) dt = 0.01f;

        // Fusión de Sensores
        float roll_acc  = atan2f(ay, az) * 180.0f / (float)M_PI;
        float pitch_acc = atan2f(-ax, sqrtf(ay*ay + az*az)) * 180.0f / (float)M_PI;

        roll_acc_f  = alpha_lp * roll_acc  + (1.0f - alpha_lp) * roll_acc_f;
        pitch_acc_f = alpha_lp * pitch_acc + (1.0f - alpha_lp) * pitch_acc_f;
        
        roll  = beta_cf * (roll  + gx * dt) + (1.0f - beta_cf) * roll_acc_f;
        pitch = beta_cf * (pitch + gy * dt) + (1.0f - beta_cf) * pitch_acc_f;
        yaw   = yaw + gz * dt; 

        char servo_status[20];
        
        if (pitch > 40.0f) {
            servo_write_us(PULSE_CW);
            sprintf(servo_status, "GIRO DCHA");
        } 
        else if (pitch < -40.0f) {
            servo_write_us(PULSE_CCW);
            sprintf(servo_status, "GIRO IZQ");
        } 
        else {
            servo_stop_hard(); // PWM = 0
            sprintf(servo_status, "STOP");
        }

        printf("ACC: %5.2f %5.2f %5.2f | GYR: %6.1f %6.1f %6.1f | POS: R=%5.1f P=%5.1f Y=%5.1f | TMP: %.1f C | SRV: %s\n",
               ax, ay, az, gx, gy, gz, roll, pitch, yaw, temp_c, servo_status);

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}