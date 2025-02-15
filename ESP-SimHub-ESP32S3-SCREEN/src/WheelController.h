#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <Bounce2.h>
#include <BleKeyboard.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

/*
INSTRUÇÕES DE CONEXÃO:

MULTIPLEXADORES CD74HC4067:
---------------------------
MUX1 (Encoders CLK/DT):
- VCC -> 3.3V
- GND -> GND
- SIG -> GPIO 1
- EN  -> GND
- S0  -> GPIO 2
- S1  -> GPIO 3
- S2  -> GPIO 4
- S3  -> GPIO 5
- Canais 0-15: Conecte CLK e DT dos encoders 1-8 em sequência
  Exemplo: 
  - Encoder1: CLK -> C0, DT -> C1
  - Encoder2: CLK -> C2, DT -> C3
  ... e assim por diante

MUX2 (Buttons):
- VCC -> 3.3V
- GND -> GND
- SIG -> GPIO 6
- EN  -> GND
- S0  -> GPIO 2 (compartilha com MUX1)
- S1  -> GPIO 3 (compartilha com MUX1)
- S2  -> GPIO 4 (compartilha com MUX1)
- S3  -> GPIO 5 (compartilha com MUX1)
- Canais 0-9: Conecte os buttons dos encoders
- Canais 10-15: Conecte os push buttons extras

SHIFT REGISTER 74HC595 (LEDs):
-----------------------------
- VCC -> 3.3V
- GND -> GND
- SER   -> GPIO 7  (DATA)
- RCLK  -> GPIO 8  (LATCH)
- SRCLK -> GPIO 9  (CLOCK)
- OE    -> GND
- Saídas Q0-Q7: Conecte os LEDs através de resistores 220Ω

JOYSTICK:
---------
- VCC -> 3.3V
- GND -> GND
- VRx -> GPIO 14 (ADC)
- VRy -> GPIO 15 (ADC)
- SW  -> GPIO 16

PADDLE SHIFTERS:
---------------
- UP   -> GPIO 17 (com resistor pullup 10k para 3.3V)
- DOWN -> GPIO 18 (com resistor pullup 10k para 3.3V)

SENSORES HALL (EMBREAGEM):
-------------------------
- VCC -> 3.3V
- GND -> GND
- LEFT  -> GPIO 19 (ADC)
- RIGHT -> GPIO 20 (ADC)

PUSH BUTTONS COM LED:
-------------------
Buttons: Conectar ao MUX2 (canais 10-15)
LEDs: Conectar às saídas do 74HC595
Cada button:
- Um terminal -> Canal do MUX
- Outro terminal -> GND
Cada LED:
- Anodo -> Saída do 74HC595 através de resistor 220Ω
- Catodo -> GND

NOTAS IMPORTANTES:
-----------------
1. Todos os buttons devem ter resistores pullup de 10k para 3.3V
2. Todos os LEDs precisam de resistores 220Ω em série
3. Mantenha os fios o mais curto possível para evitar interferência
4. Use fios de cores diferentes para facilitar a identificação
5. Recomendado usar conectores/headers para facilitar manutenção
*/

// Definição dos pinos
#define MUX1_SIG 1   // Sinal do MUX1 (Encoders)
#define MUX2_SIG 6   // Sinal do MUX2 (Buttons)
#define MUX_S0 2     // Pinos de controle compartilhados
#define MUX_S1 3
#define MUX_S2 4
#define MUX_S3 5

#define SR_DATA 7    // Shift Register
#define SR_LATCH 8
#define SR_CLOCK 9

#define JOY_X 14     // Joystick
#define JOY_Y 15
#define JOY_SW 16

#define SHIFTER_UP 17    // Paddle Shifters
#define SHIFTER_DOWN 18

#define CLUTCH_LEFT 19   // Sensores Hall
#define CLUTCH_RIGHT 20

// Configurações
#define NUM_ENCODERS 10
#define NUM_BUTTONS 10
#define DEBOUNCE_MS 5
#define ADC_RESOLUTION 12  // ESP32 tem ADC de 12 bits
#define ADC_MAX ((1 << ADC_RESOLUTION) - 1)

class WheelController {
private:
    // Objetos para controle dos componentes
    ESP32Encoder encoders[NUM_ENCODERS];
    Bounce2::Button buttons[NUM_BUTTONS];
    Bounce2::Button paddleUp;
    Bounce2::Button paddleDown;
    Bounce2::Button joystickButton;
    BleKeyboard bleKeyboard;
    USBHIDKeyboard usbKeyboard;
    bool usingBluetooth;

    // Estado dos componentes
    int16_t encoderValues[NUM_ENCODERS] = {0};
    int16_t encoderLastValues[NUM_ENCODERS] = {0};
    uint8_t buttonStates[NUM_BUTTONS] = {0};
    
    int16_t joystickX = 0;
    int16_t joystickY = 0;
    uint8_t joystickPressed = 0;
    
    int16_t clutchLeft = 0;
    int16_t clutchRight = 0;

    uint8_t ledStates[NUM_BUTTONS] = {0};  // Array para estado dos LEDs

    int16_t bitePoint = 50;  // 0-100, default 50%
    bool bitePointMode = false;

    bool dualClutchMode = true;  // true = modo F1, false = modo independente

    // Métodos auxiliares privados
    void setMuxChannel(uint8_t channel) {
        digitalWrite(MUX_S0, channel & 0x01);
        digitalWrite(MUX_S1, (channel >> 1) & 0x01);
        digitalWrite(MUX_S2, (channel >> 2) & 0x01);
        digitalWrite(MUX_S3, (channel >> 3) & 0x01);
        delayMicroseconds(5); // Tempo para estabilizar
    }

    void updateLEDs() {
        digitalWrite(SR_LATCH, LOW);
        for (int i = NUM_BUTTONS - 1; i >= 0; i--) {
            digitalWrite(SR_CLOCK, LOW);
            digitalWrite(SR_DATA, ledStates[i]);
            digitalWrite(SR_CLOCK, HIGH);
        }
        digitalWrite(SR_LATCH, HIGH);
    }

    void sendKey(uint8_t key) {
        if (usingBluetooth && bleKeyboard.isConnected()) {
            bleKeyboard.write(key);
        } else if (!usingBluetooth) {
            usbKeyboard.write(key);
        }
    }

    void setLED(uint8_t index, bool state) {
        if (index < NUM_BUTTONS) {
            ledStates[index] = state;
            updateLEDs();
        }
    }

public:
    WheelController() : 
        bleKeyboard("F1 Wheel"),
        usingBluetooth(false) {}

    void begin() {
        // Configuração dos pinos do multiplexador
        pinMode(MUX_S0, OUTPUT);
        pinMode(MUX_S1, OUTPUT);
        pinMode(MUX_S2, OUTPUT);
        pinMode(MUX_S3, OUTPUT);
        pinMode(MUX1_SIG, INPUT);
        pinMode(MUX2_SIG, INPUT);

        // Configuração do shift register
        pinMode(SR_DATA, OUTPUT);
        pinMode(SR_CLOCK, OUTPUT);
        pinMode(SR_LATCH, OUTPUT);

        // Configuração dos paddle shifters
        paddleUp.attach(SHIFTER_UP, INPUT_PULLUP);
        paddleDown.attach(SHIFTER_DOWN, INPUT_PULLUP);
        paddleUp.interval(DEBOUNCE_MS);
        paddleDown.interval(DEBOUNCE_MS);

        // Configuração do joystick
        pinMode(JOY_X, INPUT);
        pinMode(JOY_Y, INPUT);
        joystickButton.attach(JOY_SW, INPUT_PULLUP);
        joystickButton.interval(DEBOUNCE_MS);

        // Configuração dos ADC para os sensores Hall
        analogReadResolution(ADC_RESOLUTION);
        pinMode(CLUTCH_LEFT, INPUT);
        pinMode(CLUTCH_RIGHT, INPUT);

        // Inicialização dos encoders
        for (int i = 0; i < NUM_ENCODERS; i++) {
            encoders[i].attachFullQuad(
                MUX1_SIG, // CLK - será multiplexado
                MUX1_SIG  // DT - será multiplexado
            );
            buttons[i].attach(MUX2_SIG, INPUT_PULLUP); // Será multiplexado
            buttons[i].interval(DEBOUNCE_MS);
        }

        // Tenta USB primeiro
        if (ARDUINO_USB_MODE) {  // Verifica se USB está habilitado na compilação
            USB.begin();
            usbKeyboard.begin();
            usingBluetooth = false;
        } else {
            bleKeyboard.begin();
            usingBluetooth = true;
        }
    }

    void loop() {
        // Atualiza estado dos encoders e seus buttons
        for (int i = 0; i < NUM_ENCODERS; i++) {
            // Lê encoder
            setMuxChannel(i * 2); // Canal para CLK
            delayMicroseconds(5);
            setMuxChannel(i * 2 + 1); // Canal para DT
            delayMicroseconds(5);
            
            encoderValues[i] = encoders[i].getCount();

            // Lê button do encoder
            setMuxChannel(i);
            buttons[i].update();
            buttonStates[i] = buttons[i].pressed();
        }

        // Atualiza paddle shifters
        paddleUp.update();
        paddleDown.update();

        // Atualiza joystick
        joystickX = analogRead(JOY_X);
        joystickY = analogRead(JOY_Y);
        joystickButton.update();
        joystickPressed = joystickButton.pressed();

        // Atualiza sensores Hall
        clutchLeft = analogRead(CLUTCH_LEFT);
        clutchRight = analogRead(CLUTCH_RIGHT);

        // Atualiza LEDs
        updateLEDs();

        // Processa mudanças e envia comandos
        processInputChanges();
    }

    void processInputChanges() {
        if (usingBluetooth && bleKeyboard.isConnected() || !usingBluetooth) {
            // Processa encoders
            for (int i = 0; i < NUM_ENCODERS; i++) {
                if (encoderValues[i] != encoderLastValues[i]) {
                    if (encoderValues[i] > encoderLastValues[i]) {
                        sendKey(KEY_F1 + i); // Incremento
                    } else {
                        sendKey(KEY_F5 + i); // Decremento
                    }
                    encoderLastValues[i] = encoderValues[i];
                }
            }

            // Processa paddle shifters
            if (paddleUp.pressed()) sendKey(KEY_PAGE_UP);
            if (paddleDown.pressed()) sendKey(KEY_PAGE_DOWN);

            // Processa joystick como setas direcionais
            if (joystickX > ADC_MAX * 0.75) sendKey(KEY_RIGHT_ARROW);
            else if (joystickX < ADC_MAX * 0.25) sendKey(KEY_LEFT_ARROW);
            else {
                sendKey(KEY_RIGHT_ARROW);
                sendKey(KEY_LEFT_ARROW);
            }

            if (joystickY > ADC_MAX * 0.75) sendKey(KEY_UP_ARROW);
            else if (joystickY < ADC_MAX * 0.25) sendKey(KEY_DOWN_ARROW);
            else {
                sendKey(KEY_UP_ARROW);
                sendKey(KEY_DOWN_ARROW);
            }
        }
    }

    // Getters para leitura do estado
    int16_t getEncoderValue(uint8_t index) {
        return (index < NUM_ENCODERS) ? encoderValues[index] : 0;
    }

    bool getButtonState(uint8_t index) {
        return (index < NUM_BUTTONS) ? buttonStates[index] : false;
    }

    int16_t getClutchLeft() { return clutchLeft; }
    int16_t getClutchRight() { return clutchRight; }
    int16_t getJoystickX() { return joystickX; }
    int16_t getJoystickY() { return joystickY; }
    bool getJoystickButton() { return joystickPressed; }

    void setBitePoint(int16_t point) {
        bitePoint = constrain(point, 0, 100);
    }

    int16_t getBitePoint() {
        return bitePoint;
    }

    void toggleBitePointMode() {
        bitePointMode = !bitePointMode;
        // Pode usar um LED para indicar modo de ajuste do bite point
        setLED(9, bitePointMode);  // Usa último LED como indicador
    }

    void toggleClutchMode() {
        dualClutchMode = !dualClutchMode;
        setLED(8, dualClutchMode);  // LED 8 indica modo da embreagem
    }

    // Retorna valor combinado das embreagens no modo F1
    int16_t getClutchF1() {
        if (!dualClutchMode) return 0;
        
        // No modo F1, usa o valor mais alto das duas embreagens
        return max(clutchLeft, clutchRight);
    }

    // Retorna valores independentes (útil para mapeamento personalizado)
    int16_t getRawClutchLeft() {
        if (dualClutchMode) return 0;
        return clutchLeft;
    }

    int16_t getRawClutchRight() {
        if (dualClutchMode) return 0;
        return clutchRight;
    }

    // Modifica getClutchWithBitePoint para usar o modo correto
    int16_t getClutchWithBitePoint() {
        int16_t rawClutch;
        
        if (dualClutchMode) {
            rawClutch = getClutchF1();
        } else {
            // No modo independente, pode escolher qual embreagem usar com bite point
            rawClutch = clutchLeft;  // ou clutchRight, dependendo da preferência
        }
        
        if (rawClutch < bitePoint) {
            return map(rawClutch, 0, bitePoint, 0, ADC_MAX/2);
        } else {
            return map(rawClutch, bitePoint, 100, ADC_MAX/2, ADC_MAX);
        }
    }
}; 