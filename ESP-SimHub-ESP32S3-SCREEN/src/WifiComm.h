#pragma once
#define BRIDGE_PORT 10001
#include "Communication.h"
#include <TcpSerialBridge2.h>
#include <FullLoopbackStream.h>

class WifiComm : public Communication {
private:
    TcpSerialBridge2* bridge;
    FullLoopbackStream* outStream;
    FullLoopbackStream* inStream;
    Arduino_GFX* display;
    bool connected;

public:
    WifiComm(Arduino_GFX* gfx) : display(gfx) {
        bridge = new TcpSerialBridge2(BRIDGE_PORT);
        outStream = new FullLoopbackStream(1024);
        inStream = new FullLoopbackStream(1024);
        connected = false;
    }

    ~WifiComm() {
        delete bridge;
        delete outStream;
        delete inStream;
    }

    void begin() override {
        bridge->setup(outStream, inStream, display);
        connected = true;
    }

    void loop() override {
        bridge->loop();
    }

    size_t write(const uint8_t* buffer, size_t size) override {
        return outStream->write(buffer, size);
    }

    size_t read(uint8_t* buffer, size_t size) override {
        return inStream->readBytes(buffer, size);
    }

    bool isConnected() override {
        return connected && WiFi.status() == WL_CONNECTED;
    }

    const char* getName() override {
        return "WiFi";
    }
};

