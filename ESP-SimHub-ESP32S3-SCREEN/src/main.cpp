#include <Arduino.h>
#include "CommManager.h"
#include <Arduino_GFX_Library.h>
#include "LedManager.h"
#include "WheelController.h"

// Variáveis para dados do SimHub
int currentRPM = 0;
bool drsZoneActive = false;
bool drsEnabled = false;
bool yellowFlag = false;
bool blueFlag = false;

// Definições dos pinos para WT32-SC01 Plus
#define TFT_DC    7  // DC
#define TFT_CS    5  // CS
#define TFT_SCK   12 // SCK
#define TFT_MOSI  11 // MOSI
#define TFT_MISO  13 // MISO
#define TFT_RST   6  // RST

#define DEVICE_NAME "ESP-SimHubDisplay"
#define PIXEL_WIDTH 480
#define PIXEL_HEIGHT 272
#define SCREEN_WIDTH_MM 95 // only the screen area with pixels, in mm. Can be approximate, used to calculate density
#define PIXEL_PER_MM (PIXEL_WIDTH / SCREEN_WIDTH_MM)

// Mude de false para true para habilitar USB
#define INCLUDE_WIFI false  // mantenha false para usar apenas USB
// Less secure if you plan to commit or share your files, but saves a bunch of memory. 
//  If you hardcode credentials the device will only work in your network
#define USE_HARDCODED_CREDENTIALS false

#if INCLUDE_WIFI
#if USE_HARDCODED_CREDENTIALS
#define WIFI_SSID "Wifi NAME"
#define WIFI_PASSWORD "WiFi Password"
#endif

#define BRIDGE_PORT 10001 // Perle TruePort uses port 10,001 for the first serial routed to the client
#define DEBUG_TCP_BRIDGE false // emits extra events to Serial that show network communication, set to false to save memory and make faster

#include <TcpSerialBridge2.h>
#include <ECrowneWifi.h>
#include <FullLoopbackStream.h>

FullLoopbackStream outgoingStream;
FullLoopbackStream incomingStream;

#endif // INCLUDE_WIFI

#include <FlowSerialRead.h>
#include <SHCustomProtocol.h>

SHCustomProtocol shCustomProtocol;
char loop_opt;
char xactionc;
unsigned long lastSerialActivity = 0;

void idle(bool critical);


// Don't change this
#define VERSION 'j'

#include <SHCommands.h>

// Configuração do display para WT32-SC01 Plus
Arduino_DataBus *bus = new Arduino_ESP32SPI(
    TFT_DC,    // DC
    TFT_CS,    // CS
    TFT_SCK,   // SCK
    TFT_MOSI,  // MOSI
    TFT_MISO   // MISO
);

Arduino_GFX *gfx = new Arduino_ST7796(
    bus,
    TFT_RST,   // RST
    0,         // Rotation
    true       // IPS
);

CommManager commManager(gfx);

LedManager ledManager;
WheelController wheelController;

// Variável para controle de tempo
unsigned long lastWheelUpdate = 0;
const unsigned long WHEEL_UPDATE_INTERVAL = 10; // 10ms = 100Hz

void setup(void)
{
  FlowSerialBegin(19200);

  shCustomProtocol.setup();
  arqserial.setIdleFunction(idle);

#if INCLUDE_WIFI
#if DEBUG_TCP_BRIDGE
	Serial.begin(115200);
#endif
#endif

#if INCLUDE_WIFI
	ECrowneWifi::setup(&outgoingStream, &incomingStream, gfx);
#endif

  gfx->begin();
  gfx->fillScreen(BLACK);
  
  commManager.setup();

  ledManager.begin();
  ledManager.setMaxRPM(9000);  // Ajuste para o RPM máximo do seu carro

  // Inicializa o controlador do volante
  wheelController.begin();
  
  // Mensagem inicial no display
  gfx->setCursor(0, 0);
  gfx->setTextColor(WHITE);
  gfx->println("F1 Wheel + SimHub Display");
  gfx->println("Iniciando...");
}

void loop()
{
#if INCLUDE_WIFI
	ECrowneWifi::loop();
#endif

  shCustomProtocol.loop();

	if (FlowSerialAvailable() > 0) {
		if (auto r = FlowSerialTimedRead() == MESSAGE_HEADER)
		{
			lastSerialActivity = millis();
			// Read command
			loop_opt = FlowSerialTimedRead();
			switch(loop_opt) {
				case '1': Command_Hello(); break;
				case '0': Command_Features(); break;
				case '4': Command_RGBLEDSCount(); break;
				case 'X': {
					String xaction = FlowSerialReadStringUntil(' ', '\n');
					if (xaction == F("list")) Command_ExpandedCommandsList();
					else if (xaction == F("mcutype")) Command_MCUType();
				}
				break;
				case 'N': Command_DeviceName(); break;
				case 'I': Command_UniqueId(); break;
				case '8': Command_SetBaudrate(); break;
				case 'J': Command_ButtonsCount(); break;
				case '2': Command_TM1638Count(); break;
				case 'B': Command_SimpleModulesCount(); break;
				case 'A': Command_Acq(); break;
				case 'G': Command_GearData(); break;
				case 'P': Command_CustomProtocolData(); break;
				default:
					break;
			}
		}
	}

  ledManager.update();

  commManager.loop();

  // Atualização do volante em intervalo fixo
  unsigned long currentMillis = millis();
  if (currentMillis - lastWheelUpdate >= WHEEL_UPDATE_INTERVAL) {
    lastWheelUpdate = currentMillis;
    wheelController.loop();
    
    // Usar LedManager para controlar todos os LEDs
    ledManager.handleWheelEvents(drsEnabled, yellowFlag, blueFlag);
  }
}

void idle(bool critical) {
#if INCLUDE_WIFI
	yield();
	ECrowneWifi::flush();
#endif

	shCustomProtocol.idle(); 
}

void handleCustomProtocol() {
    // Atualiza RPM
    ledManager.updateRPM(currentRPM);
    
    // Atualiza flags/alertas
    ledManager.setDRSZone(drsZoneActive);
    ledManager.setDRSEnabled(drsEnabled);
    
    if(yellowFlag) ledManager.setYellowFlag();
    if(blueFlag) ledManager.setBlueFlag();
    // ... outros flags ...
}