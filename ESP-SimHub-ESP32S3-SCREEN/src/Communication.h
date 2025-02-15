#pragma once
#include <Arduino.h>

class Communication {
public:
    virtual ~Communication() {}
    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;
    virtual size_t read(uint8_t* buffer, size_t size) = 0;
    virtual bool isConnected() = 0;
    virtual const char* getName() = 0;
}; 