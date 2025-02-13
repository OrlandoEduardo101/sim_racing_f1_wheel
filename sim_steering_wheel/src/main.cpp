#include <Wire.h>
#include <Adafruit_PWMServoDriver.h> // Para controlar os LEDs via PCA9685
#include <Encoder.h> // Para ler os encoders
#include <Bounce2.h> // Para debounce dos botões
#include <TFT_eSPI.h> // Para o display LCD
#include <WiFi.h> // Para comunicação Wi-Fi com o SimHub
#include <ArduinoJson.h> // Para processar dados JSON do SimHub
#include <USB.h> // Para comunicação USB HID
#include <BluetoothSerial.h> // Para comunicação Bluetooth
#include <TouchScreen.h> // Para o touchscreen

// Definição de pinos
#define ENCODER1_PIN_A 34
#define ENCODER1_PIN_B 35
#define BUTTON1_PIN 25
#define BUTTON2_PIN 26
#define BUTTON3_PIN 27
#define BUTTON4_PIN 14
#define BUTTON5_PIN 12
#define BUTTON6_PIN 13
#define BUTTON7_PIN 15
#define BUTTON8_PIN 2
#define BUTTON9_PIN 4
#define BUTTON10_PIN 5
#define SWITCH1_PIN 18
#define SWITCH2_PIN 19
#define JOYSTICK_X_PIN 36
#define JOYSTICK_Y_PIN 39
#define HALL1_PIN 32
#define HALL2_PIN 33

// Inicialização dos componentes
Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver();
Encoder encoder1(ENCODER1_PIN_A, ENCODER1_PIN_B);
Bounce button1 = Bounce();
Bounce button2 = Bounce();
Bounce button3 = Bounce();
Bounce button4 = Bounce();
Bounce button5 = Bounce();
Bounce button6 = Bounce();
Bounce button7 = Bounce();
Bounce button8 = Bounce();
Bounce button9 = Bounce();
Bounce button10 = Bounce();
Bounce switch1 = Bounce();
Bounce switch2 = Bounce();
TFT_eSPI tft = TFT_eSPI();
BluetoothSerial SerialBT;
TouchScreen ts = TouchScreen(4, 5, 6, 7, 300); // Pinos do touchscreen

// Variáveis globais
int encoder1Value = 0;
bool button1State = false;
bool button2State = false;
bool button3State = false;
bool button4State = false;
bool button5State = false;
bool button6State = false;
bool button7State = false;
bool button8State = false;
bool button9State = false;
bool button10State = false;
bool switch1State = false;
bool switch2State = false;
int joystickXValue = 0;
int joystickYValue = 0;
int hall1Value = 0;
int hall2Value = 0;

// Configurações do menu
int ledBrightness = 100; // Brilho dos LEDs (0-255)
int lcdBrightness = 100; // Brilho do LCD (0-255)
int bitePoint = 50; // Ponto de mordida da embreagem (0-100)
bool clutchMode = false; // Modo de embreagem (false = juntas, true = independentes)

// Variáveis para o display
unsigned long lastConfigChangeTime = 0;
String currentConfigMessage = "";
int currentPage = 0; // Página atual do dashboard
unsigned long lastTouchTime = 0;

// Variáveis para integração com o SimHub
WiFiServer server(80); // Servidor Wi-Fi para receber dados do SimHub
int rpm = 0;
int speed = 0;
int gear = 0;
float fuel = 0.0;

// Variáveis para controle dos LEDs
int rpmLEDs[16] = {0}; // Estado dos LEDs de RPM (0-255)
int alertLEDs[4] = {0}; // Estado dos LEDs de alerta (0-255)
String flagColor = ""; // Cor da bandeira atual
bool drsZone = false; // Zona de DRS ativa
bool drsActive = false; // DRS ativo

void setup() {
  // Inicialização do serial
  Serial.begin(115200);

  // Inicialização do PCA9685
  pca9685.begin();
  pca9685.setPWMFreq(1600); // Frequência PWM de 1600 Hz

  // Inicialização do display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Inicialização do Bluetooth
  SerialBT.begin("ESP32-Wheel");

  // Configuração dos pinos
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  pinMode(BUTTON5_PIN, INPUT_PULLUP);
  pinMode(BUTTON6_PIN, INPUT_PULLUP);
  pinMode(BUTTON7_PIN, INPUT_PULLUP);
  pinMode(BUTTON8_PIN, INPUT_PULLUP);
  pinMode(BUTTON9_PIN, INPUT_PULLUP);
  pinMode(BUTTON10_PIN, INPUT_PULLUP);
  pinMode(SWITCH1_PIN, INPUT_PULLUP);
  pinMode(SWITCH2_PIN, INPUT_PULLUP);

  button1.attach(BUTTON1_PIN);
  button1.interval(25);
  button2.attach(BUTTON2_PIN);
  button2.interval(25);
  button3.attach(BUTTON3_PIN);
  button3.interval(25);
  button4.attach(BUTTON4_PIN);
  button4.interval(25);
  button5.attach(BUTTON5_PIN);
  button5.interval(25);
  button6.attach(BUTTON6_PIN);
  button6.interval(25);
  button7.attach(BUTTON7_PIN);
  button7.interval(25);
  button8.attach(BUTTON8_PIN);
  button8.interval(25);
  button9.attach(BUTTON9_PIN);
  button9.interval(25);
  button10.attach(BUTTON10_PIN);
  button10.interval(25);
  switch1.attach(SWITCH1_PIN);
  switch1.interval(25);
  switch2.attach(SWITCH2_PIN);
  switch2.interval(25);

  // Conectar ao Wi-Fi
  WiFi.begin("SUA_REDE_WIFI", "SUA_SENHA_WIFI");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.println("Conectando ao Wi-Fi...");
  }
  tft.println("Conectado ao Wi-Fi");

  // Iniciar servidor
  server.begin();
}

void loop() {
  // Leitura das entradas
  readEncoders();
  readButtons();
  readSwitches();
  readJoystick();
  readHallSensors();
  readTouch();

  // Receber dados do SimHub via Wi-Fi e USB
  receiveSimHubDataWiFi();
  receiveSimHubDataUSB();

  // Atualização das saídas
  updateLEDs();
  updateDisplay();

  // Verificação de configurações
  checkMenu();

  // Comunicação com o PC
  sendUSBData();
  sendBluetoothData();

  delay(10); // Pequeno delay para estabilidade
}

void readEncoders() {
  // Leitura do encoder 1
  int newEncoder1Value = encoder1.read();
  if (newEncoder1Value != encoder1Value) {
    encoder1Value = newEncoder1Value;
    // Aqui você pode adicionar lógica para o encoder 1
  }
}

void readButtons() {
  // Leitura dos botões
  button1.update();
  button2.update();
  button3.update();
  button4.update();
  button5.update();
  button6.update();
  button7.update();
  button8.update();
  button9.update();
  button10.update();

  if (button1.fell()) button1State = true;
  if (button2.fell()) button2State = true;
  if (button3.fell()) button3State = true;
  if (button4.fell()) button4State = true;
  if (button5.fell()) button5State = true;
  if (button6.fell()) button6State = true;
  if (button7.fell()) button7State = true;
  if (button8.fell()) button8State = true;
  if (button9.fell()) button9State = true;
  if (button10.fell()) button10State = true;

  if (button1.rose()) button1State = false;
  if (button2.rose()) button2State = false;
  if (button3.rose()) button3State = false;
  if (button4.rose()) button4State = false;
  if (button5.rose()) button5State = false;
  if (button6.rose()) button6State = false;
  if (button7.rose()) button7State = false;
  if (button8.rose()) button8State = false;
  if (button9.rose()) button9State = false;
  if (button10.rose()) button10State = false;
}

void readSwitches() {
  // Leitura dos switches
  switch1.update();
  switch2.update();

  if (switch1.fell()) switch1State = true;
  if (switch2.fell()) switch2State = true;

  if (switch1.rose()) switch1State = false;
  if (switch2.rose()) switch2State = false;
}

void readJoystick() {
  // Leitura do joystick
  joystickXValue = analogRead(JOYSTICK_X_PIN);
  joystickYValue = analogRead(JOYSTICK_Y_PIN);
}

void readHallSensors() {
  // Leitura dos sensores Hall
  hall1Value = analogRead(HALL1_PIN);
  hall2Value = analogRead(HALL2_PIN);
}

void readTouch() {
  // Leitura do touchscreen
  TSPoint p = ts.getPoint();
  if (p.z > ts.pressureThreshhold) {
    if (millis() - lastTouchTime > 500) { // Debounce de 500ms
      if (p.x > 1000) { // Deslize para a direita
        currentPage = (currentPage + 1) % 3; // 3 páginas no total
      } else if (p.x < 100) { // Deslize para a esquerda
        currentPage = (currentPage - 1 + 3) % 3; // 3 páginas no total
      }
      lastTouchTime = millis();
    }
  }
}

void receiveSimHubDataWiFi() {
  // Receber dados do SimHub via Wi-Fi
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String data = client.readStringUntil('\n');
        processSimHubData(data); // Processar os dados recebidos
      }
    }
    client.stop();
  }
}

void receiveSimHubDataUSB() {
  // Receber dados do SimHub via USB (Serial)
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    processSimHubData(data); // Processar os dados recebidos
  }
}

void processSimHubData(String data) {
  // Usar ArduinoJson para processar os dados
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    tft.println("Erro ao processar dados");
    return;
  }

  // Atualizar variáveis com os dados do SimHub
  rpm = doc["rpm"];
  speed = doc["speed"];
  gear = doc["gear"];
  fuel = doc["fuel"];
  flagColor = doc["flagColor"].as<String>();
  drsZone = doc["drsZone"];
  drsActive = doc["drsActive"];

  // Atualizar LEDs de RPM
  int rpmPercentage = map(rpm, 0, 10000, 0, 16); // Mapeia RPM para 16 LEDs
  for (int i = 0; i < 16; i++) {
    rpmLEDs[i] = (i < rpmPercentage) ? ledBrightness : 0;
  }

  // Atualizar LEDs de alerta
  updateAlertLEDs();
}

void updateAlertLEDs() {
  // Atualizar LEDs de alerta com base na cor da bandeira, DRS, etc.
  if (flagColor == "yellow") {
    alertLEDs[0] = ledBrightness; // LED 1: Amarelo
  } else if (flagColor == "green") {
    alertLEDs[1] = ledBrightness; // LED 2: Verde
  } else if (flagColor == "blue") {
    alertLEDs[2] = ledBrightness; // LED 3: Azul
  } else if (flagColor == "white") {
    alertLEDs[3] = ledBrightness; // LED 4: Branco
  } else if (flagColor == "black") {
    // Desliga todos os LEDs de alerta
    for (int i = 0; i < 4; i++) {
      alertLEDs[i] = 0;
    }
  }

  // Atualizar LEDs de DRS
  if (drsZone) {
    alertLEDs[0] = ledBrightness; // LED 1: Zona de DRS
  }
  if (drsActive) {
    alertLEDs[1] = ledBrightness; // LED 2: DRS ativo
  }
}

void updateLEDs() {
  // Atualização dos LEDs via PCA9685
  for (int i = 0; i < 16; i++) {
    pca9685.setPWM(i, 0, rpmLEDs[i]); // LEDs de RPM
  }
  for (int i = 0; i < 4; i++) {
    pca9685.setPWM(16 + i, 0, alertLEDs[i]); // LEDs de alerta
  }
}

void updateDisplay() {
  // Atualização do display
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  // Exibir mensagem de configuração temporária
  if (millis() - lastConfigChangeTime < 3000) { // Mostrar por 3 segundos
    tft.println(currentConfigMessage);
  } else {
    // Exibir página atual do dashboard
    switch (currentPage) {
      case 0:
        tft.println("Página 1: RPM e Velocidade");
        tft.println("RPM: " + String(rpm));
        tft.println("Velocidade: " + String(speed) + " km/h");
        break;
      case 1:
        tft.println("Página 2: Marcha e Combustível");
        tft.println("Marcha: " + String(gear));
        tft.println("Combustível: " + String(fuel) + " L");
        break;
      case 2:
        tft.println("Página 3: Informações de Corrida");
        tft.println("Tempo de Volta: 1:30.456");
        tft.println("Melhor Volta: 1:28.123");
        break;
    }
  }
}

void checkMenu() {
  // Verificação de configurações do menu
  if (button1State) {
    ledBrightness += 10;
    if (ledBrightness > 255) ledBrightness = 255;
    currentConfigMessage = "Brilho do Led: " + String(ledBrightness);
    lastConfigChangeTime = millis();
    button1State = false;
  }
  if (button2State) {
    ledBrightness -= 10;
    if (ledBrightness < 0) ledBrightness = 0;
    currentConfigMessage = "Brilho do Led: " + String(ledBrightness);
    lastConfigChangeTime = millis();
    button2State = false;
  }
  if (button3State) {
    clutchMode = !clutchMode;
    currentConfigMessage = "Modo Embreagem: " + String(clutchMode ? "Independente" : "Juntas");
    lastConfigChangeTime = millis();
    button3State = false;
  }
}

void sendUSBData() {
  // Envio de dados via USB HID
  uint8_t data[10];
  data[0] = button1State;
  data[1] = button2State;
  data[2] = button3State;
  data[3] = button4State;
  data[4] = button5State;
  data[5] = button6State;
  data[6] = button7State;
  data[7] = button8State;
  data[8] = button9State;
  data[9] = button10State;
  USB.send(data, 10);
}

void sendBluetoothData() {
  // Envio de dados via Bluetooth
  uint8_t data[10];
  data[0] = button1State;
  data[1] = button2State;
  data[2] = button3State;
  data[3] = button4State;
  data[4] = button5State;
  data[5] = button6State;
  data[6] = button7State;
  data[7] = button8State;
  data[8] = button9State;
  data[9] = button10State;
  SerialBT.write(data, 10);
}