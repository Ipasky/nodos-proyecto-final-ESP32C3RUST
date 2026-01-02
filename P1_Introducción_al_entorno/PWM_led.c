#include <stdio.h>
#include <stdio.h> 
#include "driver/gpio.h" 
#include "freertos/FreeRTOS.h" 
#include "esp_rom_sys.h" 
#include <sys/time.h> 
#include "driver/ledc.h" 
void delay_ms(int t) { 
    vTaskDelay(t / portTICK_PERIOD_MS); 
} 
void PWMconfigLow(int gpio, int chan, int timer, int res, int freq, float duty) { 
    ledc_timer_config_t ledc_timer = { .speed_mode = LEDC_LOW_SPEED_MODE, .clk_cfg = LEDC_APB_CLK }; 
    ledc_timer.timer_num = timer; 
    ledc_timer.duty_resolution = res; 
    ledc_timer.freq_hz = freq;
    ledc_channel_config_t ledc_channel = { .speed_mode = 
        LEDC_LOW_SPEED_MODE, 
        .hpoint = 0, 
        .intr_type = LEDC_INTR_DISABLE, };

     ledc_channel.channel = chan;  
     ledc_channel.timer_sel = timer;  
     ledc_channel.gpio_num = gpio;  
     ledc_channel.duty = ((float)(2 << (res - 1))) * duty;  
     ledc_timer_config(&ledc_timer);  
     ledc_channel_config(&ledc_channel); 
    }
void app_main(void) {  
    while (true)  {    
        PWMconfigLow(7, 0, 0, 8, 2000, 0.5);    
        for (int d = 0; d <= 256; d++)    {      
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 0, d);      
            ledc_update_duty(LEDC_LOW_SPEED_MODE, 0);      
            delay_ms(10);    
        }  
    } 
}
