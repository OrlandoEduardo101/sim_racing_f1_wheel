#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Encoder.h>
#include <TFT_eSPI.h>  // Biblioteca para controle do LCD do WT32-SC01 Plus
#include <TouchScreen.h>  // Biblioteca para toque
#include "BleGamepad.h"   // Biblioteca para Bluetooth Gamepad

#define NUM_LEDS 10  // LEDs de RPM/alertas
#define LED_PIN 12   // Pino dos LEDs WS2812
#define LED_BRIGHTNESS_PIN 16 // PWM para botões iluminados
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

#define ENCODER_PIN_A 34
#define ENCODER_PIN_B 35
#define ENCODER_CLICK 32
#define JOYSTICK_X 36
#define JOYSTICK_Y 39
#define JOYSTICK_BTN 25

// Configuração do touchscreen
TFT_eSPI tft = TFT_eSPI();
uint16_t touchX, touchY;

// LEDs endereçáveis
Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Controle de encoders
Encoder encoder1(ENCODER_PIN_A, ENCODER_PIN_B);
int brightness = 128;  // Brilho inicial

// Inicializa Bluetooth Gamepad
BleGamepad bleGamepad("F1 Wheel", "Custom", 100);

void setup() {
    Serial.begin(115200);

    // Configurações de PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED_BRIGHTNESS_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, brightness);

    // Inicializa LEDs
    leds.begin();
    leds.setBrightness(50);
    leds.show();

    // Configura botões e encoders
    pinMode(ENCODER_CLICK, INPUT_PULLUP);
    pinMode(JOYSTICK_BTN, INPUT_PULLUP);

    // Inicializa LCD e Touch
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("F1 Wheel Ready!", 50, 20);

    // Inicia Bluetooth
    bleGamepad.begin();
}

void loop() {
    // Leitura do encoder
    long newPos = encoder1.read();
    Serial.print("Encoder Pos: ");
    Serial.println(newPos);

    // Controle de LEDs RGB (RPM)
    for (int i = 0; i < NUM_LEDS; i++) {
        if (i < (newPos / 10)) {
            leds.setPixelColor(i, leds.Color(255, 0, 0)); // Vermelho
        } else {
            leds.setPixelColor(i, leds.Color(0, 0, 0)); // Apaga
        }
    }
    leds.show();

    // Controle de brilho via encoder
    if (digitalRead(ENCODER_CLICK) == LOW) {
        brightness = min(brightness + 10, 255);
        ledcWrite(PWM_CHANNEL, brightness);
    }

    // Leitura do touchscreen
    if (tft.getTouch(&touchX, &touchY)) {
        Serial.printf("Toque detectado em: %d, %d\n", touchX, touchY);

        // Exemplo: Alterar página do LCD ao toque
        if (touchX > 200 && touchY < 100) {
            tft.fillScreen(TFT_BLUE);
            tft.drawString("Pagina 2", 50, 20);
        }
    }

    // Controle do Bluetooth Gamepad
    if (bleGamepad.isConnected()) {
        bleGamepad.setX(analogRead(JOYSTICK_X) / 16);  // Normaliza valores
        bleGamepad.setY(analogRead(JOYSTICK_Y) / 16);
        if (digitalRead(JOYSTICK_BTN) == LOW) {
            bleGamepad.press(BUTTON_1);
        } else {
            bleGamepad.release(BUTTON_1);
        }
    }

    delay(100);
}
