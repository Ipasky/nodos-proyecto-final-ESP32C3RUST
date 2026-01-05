
#include <stdint.h>
#include <string.h>
#include "esp32c3/rom/ets_sys.h"
// direcciones base de los GPIOs
#define GPIO_BASE        0x60004000
#define GPIO_OUT_W1TS    (GPIO_BASE + 0x08)
#define GPIO_OUT_W1TC    (GPIO_BASE + 0x0C)
#define GPIO_ENABLE_W1TS (GPIO_BASE + 0x20)

#define ZUMBADOR 2 
#define DELAY_US 1    // tiempo base para el PWM

// registros GPIO
volatile uint32_t* gpio_out_w1ts = (volatile uint32_t*) GPIO_OUT_W1TS;
volatile uint32_t* gpio_out_w1tc = (volatile uint32_t*) GPIO_OUT_W1TC;
volatile uint32_t* gpio_enable_w1ts = (volatile uint32_t*) GPIO_ENABLE_W1TS;

// ESTRUCTURA: nombre y frecuencia fundamental de las notas
typedef struct {
    const char *nombre;
    int freq;   // frecuencia en Hz
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

// Estructura: nombre y duración de la nota 
typedef struct {
    const char *nombre;
    int duracion_ms;
} NotaPartitura;

NotaPartitura partitura[] = {
    {"DO", 400}, {"DO", 400}, {"SOL", 400}, {"SOL", 400},
    {"LA", 400}, {"LA", 400}, {"SOL", 800},
    {"FA", 400}, {"FA", 400}, {"MI", 400}, {"MI", 400},
    {"RE", 400}, {"MI", 400}, {"DO", 800}
};

int longPartitura = sizeof(partitura)/sizeof(NotaPartitura);
int numNotas = sizeof(notas)/sizeof(Nota);

// Busca índice de nota
int Indice(const char *nombre) {
    for (int i = 0; i < numNotas; i++) {
        if (strcmp(nombre, notas[i].nombre) == 0) return i;
    }
    return -1;
}

// Retardo en milisegundos para las pausas
void delay_ms(int t) {
    ets_delay_us(t * 1000);
}

// Genera las notas en función de la frecuencia
void play_nota(const char *nota, int duracion_ms) {
    int freq = notas[Indice(nota)].freq;
    if (freq <= 0) return;

    int periodo_us = 1000000 / freq;  // periodo de la onda
    int mitad_periodo = periodo_us / 2; // 50% duty cycle
    int repeticiones = (duracion_ms * 1000) / periodo_us;

    for (int i = 0; i < repeticiones; i++) {
        // Encender pin
        *gpio_out_w1ts = (1 << ZUMBADOR);
        ets_delay_us(mitad_periodo);

        // Apagar pin
        *gpio_out_w1tc = (1 << ZUMBADOR);
        ets_delay_us(mitad_periodo);
    }

    // pausa entre notas
    delay_ms(50);
}

void app_main(void) {
    // Configurar pin como salida
    *gpio_enable_w1ts = (1 << BUZZER_PIN);

    while (1) {
        for (int i = 0; i < longPartitura; i++) {
            play_nota(partitura[i].nombre, partitura[i].duracion_ms);
        }

        // pausa entre repetición de la canción
        delay_ms(200);
    }
}
