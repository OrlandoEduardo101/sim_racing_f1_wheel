#pragma once
#include "Communication.h"
#include "USB.h"
#include "USBCDC.h"

class UsbComm : public Communication {
private:
    USBCDC* usb;
    bool connected;

public:
    UsbComm() {
        usb = new USBCDC(0);
        connected = false;
    }

    ~UsbComm() {
        delete usb;
    }

    void begin() override {
        USB.begin();
        usb->begin();
        connected = true;
    }

    void loop() override {
        // USB CDC não precisa de loop específico
    }

    size_t write(const uint8_t* buffer, size_t size) override {
        return usb->write(buffer, size);
    }

    size_t read(uint8_t* buffer, size_t size) override {
        return usb->read(buffer, size);
    }

    bool isConnected() override {
        return connected;  // Por enquanto, só verifica se a conexão inicial foi feita
    }

    const char* getName() override {
        return "USB";
    }
}; 