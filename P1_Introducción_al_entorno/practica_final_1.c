
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

// ---------- PIN Y PWM ----------
#define GPIO_PWM 7
#define PWM_CHANNEL 0
#define PWM_TIMER 0
#define PWM_RES 8   // resolución de 8 bits

// ---------- DEFINICIÓN DE NOTAS ----------
typedef struct {
    char *nombre;
    int freq;   // Frecuencia de la nota en Hz

} Nota;

Nota notas[] = {
    {"DO", 261},
    {"RE", 294},
    {"MI", 329},
    {"FA", 349},
    {"SOL", 392},
    {"LA", 440},
    {"SI", 493},
    {"DO5", 523}
};
typedef struct {
    const char *nombre; // Nombre de la nota
    int duracion_ms;    // Duración en milisegundos
} NotaPartitura;
NotaPartitura partitura[] = {
    {"DO", 400}, {"DO", 400}, {"SOL", 400}, {"SOL", 400},
    {"LA", 400}, {"LA", 400}, {"SOL", 800},
    {"FA", 400}, {"FA", 400}, {"MI", 400}, {"MI", 400},
    {"RE", 400}, {"MI", 400}, {"DO", 800}
};

int longPartitura = sizeof(partitura) / sizeof(NotaPartitura);


int numNotas = sizeof(notas)/sizeof(Nota);

// FUNCIONES 

// busca la nota en la partitura y devuelve su índice
int Indice(const char *nombre) {
    for (int i = 0; i < numNotas; i++) {
        if (strcmp(nombre, notas[i].nombre) == 0) {
            return i;
        }
    }
    return -1; // si no se encuentra
}

// Función para retraso en ms
void pausa(int t) {
    vTaskDelay(t / portTICK_PERIOD_MS);
}

// Función para reproducir una nota con PWM y duty cycle 50%
void play_nota(const char *nota, int duracion) {
    int freq = notas[Indice(nota)].freq;

    // Configurar timer LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = PWM_TIMER,
        .duty_resolution = PWM_RES,
        .freq_hz = freq,
        .clk_cfg = LEDC_APB_CLK
    };

    // Configurar canal LEDC
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_PWM,
        .duty = (2 << (PWM_RES - 1)) / 2  // duty cycle fijo 50%
    };

    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);

    pausa(duracion);

    // Apagar la nota
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);

    // pausa entre las notas
    pausa(50);
}


void app_main(void) {

    // PWM salida 
    gpio_set_direction(GPIO_PWM, GPIO_MODE_OUTPUT);

    while (1) {
        //partitura en bucle
        for (int i = 0; i < longPartitura; i++) {
            play_nota(partitura[i].nombre, partitura[i].duracion_ms);
            printf("Nota: %s\n", partitura[i].nombre);
        }

        //silencio entre repeticiones
        pausa(200);
    }
}
