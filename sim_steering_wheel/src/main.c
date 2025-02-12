#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Encoder.h>
#include <TFT_eSPI.h>  // Biblioteca para controle do LCD do WT32-SC01 Plus
#include <TouchScreen.h>  // Biblioteca para toque
#include "BleGamepad.h"   // Biblioteca para Bluetooth Gamepad
#include <Adafruit_PWMServoDriver.h> // PCA9685 para controle de LEDs

// Definições de hardware
#define NUM_LEDS 16  // LEDs de RPM
#define LED_PIN 12   // Pino dos LEDs WS2812
#define ENCODER_SELECT_A 34
#define ENCODER_SELECT_B 35
#define ENCODER_ADJUST_A 32
#define ENCODER_ADJUST_B 33
#define ENCODER_CLICK 26
#define JOYSTICK_X 36
#define JOYSTICK_Y 39
#define JOYSTICK_BTN 25
#define CLUTCH_SENSOR_L 27
#define CLUTCH_SENSOR_R 26
#define SHIFT_UP 13
#define SHIFT_DOWN 14
#define LED_BRIGHTNESS_CHANNEL 0
#define LED_BRIGHTNESS_FREQ 5000
#define LED_BRIGHTNESS_RES 8

// Objetos de controle
TFT_eSPI tft = TFT_eSPI();
Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
Encoder encoderSelect(ENCODER_SELECT_A, ENCODER_SELECT_B);
Encoder encoderAdjust(ENCODER_ADJUST_A, ENCODER_ADJUST_B);
BleGamepad bleGamepad("F1 Wheel", "Custom", 100);

// Configurações disponíveis
const char *configOptions[] = {
    "Brilho LEDs", "Brilho Tela", "Bite Point", "Modo Joystick",
    "Modo Embreagem", "Volume", "Sensibilidade Direcao", "Reatividade Freio",
    "Forca do FFB", "Modo Troca de Marcha", "Estabilidade", "ABS",
    "Controle de Tração", "Distribuição de Frenagem", "Gerenciamento de ERS",
    "Modo Motor", "Consumo de Combustivel", "Pressão Pneus", "Config Pessoal 1",
    "Config Pessoal 2"
};
#define NUM_CONFIG_OPTIONS (sizeof(configOptions) / sizeof(configOptions[0]))

// Variáveis globais
int configOption = 0;
int configValues[NUM_CONFIG_OPTIONS] = {128};
bool mouseMode = false;
bool clutchCombined = true;
int page = 1;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    pwm.begin();
    pwm.setPWMFreq(1600);

    ledcSetup(LED_BRIGHTNESS_CHANNEL, LED_BRIGHTNESS_FREQ, LED_BRIGHTNESS_RES);
    ledcAttachPin(LED_PIN, LED_BRIGHTNESS_CHANNEL);

    leds.begin();
    leds.setBrightness(50);
    leds.show();

    pinMode(ENCODER_CLICK, INPUT_PULLUP);
    pinMode(JOYSTICK_BTN, INPUT_PULLUP);
    pinMode(SHIFT_UP, INPUT_PULLUP);
    pinMode(SHIFT_DOWN, INPUT_PULLUP);
    pinMode(CLUTCH_SENSOR_L, INPUT);
    pinMode(CLUTCH_SENSOR_R, INPUT);

    tft.init();
    tft.setRotation(1);
    updateScreen();

    bleGamepad.begin();
}

void loop() {
    handleEncoders();
    handleTouch();
    handleJoystick();
    handleShifters();
    handleClutch();
    updateLEDs();
    delay(100);
}

void handleEncoders() {
    long selectPos = encoderSelect.read() / 4;
    long adjustPos = encoderAdjust.read() / 4;
    
    configOption = abs(selectPos) % NUM_CONFIG_OPTIONS;
    configValues[configOption] = constrain(configValues[configOption] + adjustPos, 0, 255);
    
    encoderAdjust.write(0);
    updateScreen();
}

void handleTouch() {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        if (x > 200 && y < 100) {
            page = (page % 3) + 1;
            updateScreen();
        }
    }
}

void handleJoystick() {
    if (mouseMode) {
        int moveX = analogRead(JOYSTICK_X) - 2048;
        int moveY = analogRead(JOYSTICK_Y) - 2048;
        bleGamepad.setX(moveX / 16);
        bleGamepad.setY(moveY / 16);
    }
    if (digitalRead(JOYSTICK_BTN) == LOW) {
        mouseMode = !mouseMode;
    }
}

void handleShifters() {
    if (digitalRead(SHIFT_UP) == LOW) bleGamepad.press(BUTTON_2);
    else bleGamepad.release(BUTTON_2);
    if (digitalRead(SHIFT_DOWN) == LOW) bleGamepad.press(BUTTON_3);
    else bleGamepad.release(BUTTON_3);
}

void handleClutch() {
    int clutchL = analogRead(CLUTCH_SENSOR_L) / 16;
    int clutchR = analogRead(CLUTCH_SENSOR_R) / 16;
    int clutchValue = clutchCombined ? max(clutchL, clutchR) : (clutchL + clutchR) / 2;

    if (clutchValue < configValues[2]) { // Bite Point
        clutchValue = 0;
    }
    bleGamepad.setZ(clutchValue);
}

void updateLEDs() {
    for (int i = 0; i < NUM_LEDS; i++) {
        if (i < (configValues[0] / 16)) {
            leds.setPixelColor(i, leds.Color(255, 0, 0));
        } else {
            leds.setPixelColor(i, leds.Color(0, 0, 0));
        }
    }
    leds.show();
}

void updateScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(50, 20);
    tft.printf("Pagina: %d", page);
    tft.setCursor(50, 50);
    tft.printf("Config: %s", configOptions[configOption]);
    tft.setCursor(50, 80);
    tft.printf("Valor: %d", configValues[configOption]);
}
