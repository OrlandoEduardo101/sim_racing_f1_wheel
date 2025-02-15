#pragma once
#include <FastLED.h>
#include "Config.h"

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

class LedManager {
private:
    CRGB leds[NUM_LEDS];
    int maxRPM = 0;
    int rpmValue = 0;
    int currentBrightness = 10;  // Começa com 50% do brilho
    
    // Variáveis para controle do pisca
    bool isBlinking = false;
    CRGB blinkColor = CRGB::Black;
    unsigned long lastBlinkTime = 0;
    const unsigned long BLINK_INTERVAL = 500; // Intervalo de 500ms (meio segundo)

public:
    void begin() {
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        setBrightness(10);  // Inicia com 50% do brilho
        clearAll();
    }

    void clearAll() {
        FastLED.clear();
        FastLED.show();
    }

    void setMaxRPM(int rpm) {
        maxRPM = rpm;
    }

    void updateRPM(int rpm) {
        rpmValue = rpm;
        int ledsToLight = map(rpm, 0, maxRPM, 0, RPM_LEDS);
        
        // Cores para a barra de RPM
        for(int i = RPM_START_LED; i <= RPM_END_LED; i++) {
            int rpmIndex = i - RPM_START_LED;
            if(rpmIndex < ledsToLight) {
                if(rpmIndex < RPM_LEDS * 0.7)      // Verde até 70%
                    leds[i] = CRGB::Green;
                else if(rpmIndex < RPM_LEDS * 0.9)  // Amarelo até 90%
                    leds[i] = CRGB::Yellow;
                else                                // Vermelho após 90%
                    leds[i] = CRGB::Red;
            } else {
                leds[i] = CRGB::Black;
            }
        }
        FastLED.show();
    }

    void update() {
        // Atualiza o estado do pisca-pisca se estiver ativo
        if (isBlinking) {
            unsigned long currentTime = millis();
            if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
                static bool blinkState = false;
                blinkState = !blinkState;
                
                CRGB colorToShow = blinkState ? blinkColor : CRGB::Black;
                setAlert(colorToShow);
                
                lastBlinkTime = currentTime;
            }
        }
    }

    void startBlink(CRGB color) {
        isBlinking = true;
        blinkColor = color;
        lastBlinkTime = millis();
    }

    void stopBlink() {
        isBlinking = false;
        setAlert(CRGB::Black);
    }

    // Novo método para atualizar todos os LEDs de alerta
    void setAlert(CRGB color) {
        // Atualiza primeiro grupo de alerta (LEDs 0-3)
        for(int i = ALERT_START_1; i <= ALERT_END_1; i++) {
            leds[i] = color;
        }
        
        // Atualiza segundo grupo de alerta (LEDs 20-23)
        for(int i = ALERT_START_2; i <= ALERT_END_2; i++) {
            leds[i] = color;
        }
        FastLED.show();
    }

    // Métodos para diferentes tipos de alerta
    void setYellowFlag() { startBlink(CRGB::Yellow); }
    void setBlueFlag() { startBlink(CRGB::Blue); }
    void setRedFlag() { startBlink(CRGB::Red); }
    void setGreenFlag() { startBlink(CRGB::Green); }
    void setWhiteFlag() { startBlink(CRGB::White); }
    void setPurpleFlag() { startBlink(CRGB::Purple); }
    void setDRSZone(bool active) { 
        if(active) {
            isBlinking = false;
            setAlert(CRGB::Purple);
        } else {
            clearAlert();
        }
    }
    void setDRSEnabled(bool active) { 
        if(active) {
            isBlinking = false;
            setAlert(CRGB::Blue);
        } else {
            clearAlert();
        }
    }
    void clearAlert() {
        isBlinking = false;
        setAlert(CRGB::Black);
    }

    // Novo método para controle de brilho
    void setBrightness(int level) {
        // Limita o nível entre 0 e 20
        level = constrain(level, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        currentBrightness = level;
        
        // limitador para evitar do led trabalhar no seu maximo e economizar vida util
        if(currentBrightness >= 18){
            currentBrightness = 18;
            level = 18;
        }
        
        // Converte a escala 0-20 para 0-255 que o FastLED usa
        int fastLedBrightness = map(level, 0, 20, 0, 255);
        // Ajustado para que 20 seja realmente 100% (255)
        //uint8_t fastLedBrightness = (level * 255) / 20;
        FastLED.setBrightness(fastLedBrightness);
        FastLED.show();
    }

    // Método para obter o brilho atual
    int getBrightness() {
        return currentBrightness;
    }

    // Métodos para ajuste incremental do brilho
    void increaseBrightness() {
        setBrightness(currentBrightness + 1);
    }

    void decreaseBrightness() {
        setBrightness(currentBrightness - 1);
    }
}; 