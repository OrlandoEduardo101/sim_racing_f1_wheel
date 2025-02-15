#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "Config.h"

/*
 * GUIA DE INSTALAÇÃO DOS LEDS COM PCA9685 E IRLZ34N
 * ------------------------------------------------
 * 
 * 1. Conexões do PCA9685:
 *    - VCC  -> 3.3V
 *    - GND  -> GND
 *    - SDA  -> GPIO 22 (I2C)
 *    - SCL  -> GPIO 23 (I2C)
 *    - OE   -> GND (sempre habilitado)
 * 
 * 2. Para cada LED de botão:
 *    Canal PCA9685 -> IRLZ34N -> LED
 *    Exemplo para um LED:
 *    - Canal PWM do PCA9685 -> Resistor 220Ω -> Gate do IRLZ34N
 *    - Drain do IRLZ34N -> Cátodo do LED
 *    - Ânodo do LED -> 3.3V
 *    - Source do IRLZ34N -> GND
 *    - Resistor 10k entre Gate e GND (pull-down)
 * 
 * 3. Para os WS2812B (LEDs endereçáveis):
 *    - VDD  -> 5V
 *    - GND  -> GND
 *    - DIN  -> GPIO 21 (através do conversor de nível)
 *    - DOUT -> DIN do próximo LED
 * 
 * NOTAS IMPORTANTES:
 * ----------------
 * 1. Alimentação:
 *    - Use fonte 5V/2A para os WS2812B
 *    - PCA9685 e LEDs simples podem usar 3.3V do ESP32
 * 
 * 2. Proteção:
 *    - Capacitor 100µF entre VCC e GND do PCA9685
 *    - Capacitor 1000µF na alimentação dos WS2812B
 *    - Resistores pull-down de 10k em cada Gate
 * 
 * 3. Considerações:
 *    - O PCA9685 tem resolução de 12 bits (4096 níveis)
 *    - Frequência PWM de 1000Hz é boa para evitar flicker
 *    - IRLZ34N suporta controle com 3.3V no Gate
 *    - Mantenha os fios de dados curtos e organizados
 */

/*
 * GUIA DE INSTALAÇÃO DOS WS2812B COM TXS0108E E CONTROLE DE BRILHO
 * --------------------------------------------------------------
 * 
 * 1. Conexões do TXS0108E (Conversor de Nível):
 *    Lado A (3.3V):
 *    - VccA -> 3.3V do ESP32
 *    - A1   -> Pino 21 do ESP32 (LED_PIN)
 *    - OE   -> 3.3V
 *    - GND  -> GND
 * 
 *    Lado B (5V):
 *    - VccB -> 5V
 *    - B1   -> DIN do primeiro WS2812B
 *    - GND  -> GND
 * 
 * 2. Controle de Brilho com IRLZ34N:
 *    - Gate   -> Pino PWM do ESP32 (ex: GPIO 4)
 *    - Drain  -> GND dos WS2812B
 *    - Source -> GND da fonte
 *    - Resistor 10k entre Gate e GND (pull-down)
 * 
 * 3. Conexões WS2812B:
 *    - VDD  -> 5V
 *    - GND  -> Drain do MOSFET
 *    - DIN  -> B1 do TXS0108E
 *    - DOUT -> DIN do próximo LED
 * 
 * LEMBRETES IMPORTANTES
 * -------------------
 * 1. Fonte de Alimentação:
 *    - Use fonte 5V adequada para os LEDs
 *    - Cada LED pode consumir até 60mA em brilho máximo
 *    - Para 24 LEDs, considere fonte de pelo menos 2A
 * 
 * 2. Componentes Necessários:
 *    - MOSFET IRLZ34N
 *    - Resistor 10k (pull-down para o Gate)
 *    - Capacitor 1000µF entre 5V e GND
 *    - Resistor 470Ω entre TXS0108E e primeiro LED
 * 
 * 3. Características do IRLZ34N:
 *    - Logic-level MOSFET (funciona com 3.3V)
 *    - Suporta até 30A (mais que suficiente)
 *    - RDS(on) muito baixo
 * 
 * 4. Dicas de Instalação:
 *    - Mantenha os fios de dados curtos
 *    - Use capacitores de desacoplamento 0.1µF
 *    - O MOSFET pode precisar de dissipador se usar brilho alto
 *    - Faça conexões sólidas
 */

// Definições para LEDs
#define NUM_LEDS 16          // LEDs endereçáveis para RPM
#define LED_PIN 21           // Pino para dados dos LEDs endereçáveis
#define NUM_BUTTON_LEDS 10   // LEDs simples dos botões
#define MAX_BRIGHTNESS 255   // Brilho máximo

// Endereço I2C do PCA9685
#define PCA9685_I2C_ADDRESS 0x40

class LedManager {
private:
    CRGB leds[NUM_LEDS];
    Adafruit_PWM_Servo_Driver pwm;
    
    int maxRPM;
    int currentRPM;
    bool drsZone;
    bool drsEnabled;
    bool yellowFlagActive;
    bool blueFlagActive;
    
    uint8_t buttonLedBrightness[NUM_BUTTON_LEDS];

    // Cores predefinidas
    const CRGB COLOR_RPM_LOW = CRGB::Green;
    const CRGB COLOR_RPM_MID = CRGB::Yellow;
    const CRGB COLOR_RPM_HIGH = CRGB::Red;
    const CRGB COLOR_DRS = CRGB::Purple;
    const CRGB COLOR_YELLOW_FLAG = CRGB::Yellow;
    const CRGB COLOR_BLUE_FLAG = CRGB::Blue;

public:
    LedManager() : 
        pwm(PCA9685_I2C_ADDRESS),
        maxRPM(0),
        currentRPM(0),
        drsZone(false),
        drsEnabled(false),
        yellowFlagActive(false),
        blueFlagActive(false) {
            memset(buttonLedBrightness, 0, sizeof(buttonLedBrightness));
    }

    void begin() {
        // Inicializa LEDs endereçáveis
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        FastLED.setBrightness(MAX_BRIGHTNESS);
        
        // Inicializa PCA9685
        pwm.begin();
        pwm.setPWMFreq(1000);  // Frequência PWM de 1KHz
        
        // Configura todos os canais do PCA9685 para 0
        for (int i = 0; i < 16; i++) {
            pwm.setPWM(i, 0, 0);
        }
        
        // Limpa todos os LEDs
        clearAll();
    }

    void setMaxRPM(int rpm) {
        maxRPM = rpm;
    }

    void updateRPM(int rpm) {
        currentRPM = rpm;
        updateRPMLeds();
    }

    void setButtonLED(uint8_t index, uint8_t brightness) {
        if (index < NUM_BUTTON_LEDS) {
            buttonLedBrightness[index] = brightness;
            // Mapeia o brilho de 0-255 para 0-4095 (resolução do PCA9685)
            uint16_t pwmValue = map(brightness, 0, 255, 0, 4095);
            pwm.setPWM(index, 0, pwmValue);
        }
    }

    void setDRSZone(bool active) {
        drsZone = active;
        updateDRSLeds();
    }

    void setDRSEnabled(bool enabled) {
        drsEnabled = enabled;
        updateDRSLeds();
    }

    void setYellowFlag() {
        yellowFlagActive = true;
        updateFlagLeds();
    }

    void setBlueFlag() {
        blueFlagActive = true;
        updateFlagLeds();
    }

    void clearFlags() {
        yellowFlagActive = false;
        blueFlagActive = false;
        updateFlagLeds();
    }

    void update() {
        updateRPMLeds();
        updateDRSLeds();
        updateFlagLeds();
        FastLED.show();
    }

    void handleWheelEvents(bool drsActive, bool yellowFlag, bool blueFlag) {
        // LED 0 para DRS
        setButtonLED(0, drsActive ? MAX_BRIGHTNESS : 0);
        
        // LED 1 para bandeira amarela
        setButtonLED(1, yellowFlag ? MAX_BRIGHTNESS : 0);
        
        // LED 2 para bandeira azul
        setButtonLED(2, blueFlag ? MAX_BRIGHTNESS : 0);
    }

private:
    void updateRPMLeds() {
        int numLedsToLight = map(currentRPM, 0, maxRPM, 0, NUM_LEDS);
        
        for (int i = 0; i < NUM_LEDS; i++) {
            if (i < numLedsToLight) {
                // Define cor baseada na porcentagem do RPM
                float rpmPercent = (float)i / NUM_LEDS;
                if (rpmPercent < 0.5) {
                    leds[i] = COLOR_RPM_LOW;
                } else if (rpmPercent < 0.8) {
                    leds[i] = COLOR_RPM_MID;
                } else {
                    leds[i] = COLOR_RPM_HIGH;
                }
            } else {
                leds[i] = CRGB::Black;
            }
        }
    }

    void updateDRSLeds() {
        // Usa os últimos 2 LEDs para indicação de DRS
        if (drsZone) {
            if (drsEnabled) {
                leds[NUM_LEDS-1] = COLOR_DRS;
                leds[NUM_LEDS-2] = COLOR_DRS;
            } else {
                leds[NUM_LEDS-1] = COLOR_DRS;
                leds[NUM_LEDS-2] = CRGB::Black;
            }
        } else {
            leds[NUM_LEDS-1] = CRGB::Black;
            leds[NUM_LEDS-2] = CRGB::Black;
        }
    }

    void updateFlagLeds() {
        // Usa os primeiros LEDs para flags
        if (yellowFlagActive) {
            leds[0] = COLOR_YELLOW_FLAG;
            leds[1] = COLOR_YELLOW_FLAG;
        } else if (blueFlagActive) {
            leds[0] = COLOR_BLUE_FLAG;
            leds[1] = COLOR_BLUE_FLAG;
        } else {
            // Retorna ao padrão de RPM se não houver flags
            updateRPMLeds();
        }
    }

    void clearAll() {
        // Limpa LEDs endereçáveis
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        
        // Limpa LEDs dos botões
        for (int i = 0; i < NUM_BUTTON_LEDS; i++) {
            setButtonLED(i, 0);
        }
    }
}; 