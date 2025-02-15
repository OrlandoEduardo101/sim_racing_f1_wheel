#pragma once
#include "Communication.h"
#include "WifiComm.h"
#include "UsbComm.h"
#include "BluetoothComm.h"
#include <Arduino_GFX_Library.h>

class CommManager {
private:
    Communication* activeComm;
    Arduino_GFX* display;
    
public:
    CommManager(Arduino_GFX* gfx) : display(gfx), activeComm(nullptr) {}
    
    ~CommManager() {
        if (activeComm) {
            delete activeComm;
        }
    }

    void setup() {
        // Tenta USB primeiro
        activeComm = new UsbComm();
        activeComm->begin();
        
        if (!activeComm->isConnected()) {
            delete activeComm;
            
            // Tenta Bluetooth
            activeComm = new BluetoothComm();
            activeComm->begin();
            
            if (!activeComm->isConnected()) {
                delete activeComm;
                
                // Por último, tenta WiFi
                activeComm = new WifiComm(display);
                activeComm->begin();
            }
        }

        if (display) {
            display->setCursor(0, 0);
            display->print("Connected via: ");
            display->println(activeComm->getName());
        }
    }
    
    void loop() {
        if (activeComm) {
            activeComm->loop();
        }
    }

    Communication* getCurrent() {
        return activeComm;
    }
}; 