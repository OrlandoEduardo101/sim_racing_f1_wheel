#pragma once

#define BRIDGE_PORT 10001
#define DEBUG_TCP_BRIDGE false 

// LED Configurations
#define LED_PIN         21    // Pino que vai ao TXS0108E
#define NUM_LEDS        24    // Total de LEDs

// Configuração do RPM
#define RPM_START_LED   4     // Primeiro LED do RPM
#define RPM_END_LED     19    // Último LED do RPM
#define RPM_LEDS        16    // Total de LEDs para RPM (do 4 ao 19)

// Configuração dos LEDs de alerta
// Grupo 1: LEDs 0-3
#define ALERT_START_1   0     // Primeiro grupo de alerta começa no LED 0
#define ALERT_END_1     3     // Primeiro grupo de alerta termina no LED 3

// Grupo 2: LEDs 20-23
#define ALERT_START_2   20    // Segundo grupo de alerta começa no LED 20
#define ALERT_END_2     23    // Segundo grupo de alerta termina no LED 23

// Índices dos LEDs de alerta
// #define DRS_ZONE_LED    16    // LED para zona de DRS
// #define DRS_ENABLED_LED 17    // LED para DRS ativado
// #define YELLOW_FLAG_LED 18    // LED para bandeira amarela
// #define BLUE_FLAG_LED   19    // LED para bandeira azul
// #define RED_FLAG_LED    20    // LED para bandeira vermelha
// #define GREEN_FLAG_LED  21    // LED para bandeira verde
// #define WHITE_FLAG_LED  22    // LED para última volta
// #define PURPLE_FLAG_LED 23    // LED para setor mais rápido

// Configuração de Brilho
#define BRIGHTNESS_PIN   4     // Pino PWM para o MOSFET
#define MIN_BRIGHTNESS   0     // Brilho mínimo (0%)
#define MAX_BRIGHTNESS   20    // Brilho máximo (100%) 