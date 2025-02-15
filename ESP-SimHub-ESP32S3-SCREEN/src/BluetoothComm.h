#pragma once
#include "Communication.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class BluetoothComm : public Communication {
private:
    BLEServer* pServer;
    BLECharacteristic* pTxCharacteristic;
    bool deviceConnected;
    std::vector<uint8_t> rxBuffer;

    class ServerCallbacks: public BLEServerCallbacks {
        BluetoothComm* comm;
    public:
        ServerCallbacks(BluetoothComm* c) : comm(c) {}
        void onConnect(BLEServer* pServer) override {
            comm->deviceConnected = true;
        }
        void onDisconnect(BLEServer* pServer) override {
            comm->deviceConnected = false;
        }
    };

    class CharacteristicCallbacks: public BLECharacteristicCallbacks {
        BluetoothComm* comm;
    public:
        CharacteristicCallbacks(BluetoothComm* c) : comm(c) {}
        void onWrite(BLECharacteristic* pCharacteristic) override {
            std::string value = pCharacteristic->getValue();
            if (value.length() > 0) {
                for (int i = 0; i < value.length(); i++) {
                    comm->rxBuffer.push_back(value[i]);
                }
            }
        }
    };

public:
    BluetoothComm() : deviceConnected(false) {}

    void begin() override {
        BLEDevice::init("SimHub Dashboard");
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        BLEService* pService = pServer->createService(SERVICE_UUID);
        pTxCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID_TX,
            BLECharacteristic::PROPERTY_NOTIFY
        );
        pTxCharacteristic->addDescriptor(new BLE2902());

        BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID_RX,
            BLECharacteristic::PROPERTY_WRITE
        );
        pRxCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

        pService->start();
        pServer->getAdvertising()->start();
    }

    void loop() override {
        if (!deviceConnected) {
            pServer->getAdvertising()->start();
            delay(500);
        }
    }

    size_t write(const uint8_t* buffer, size_t size) override {
        if (deviceConnected) {
            pTxCharacteristic->setValue((uint8_t*)buffer, size);
            pTxCharacteristic->notify();
            return size;
        }
        return 0;
    }

    size_t read(uint8_t* buffer, size_t size) override {
        size_t read = 0;
        while (!rxBuffer.empty() && read < size) {
            buffer[read++] = rxBuffer.front();
            rxBuffer.erase(rxBuffer.begin());
        }
        return read;
    }

    bool isConnected() override {
        return deviceConnected;
    }

    const char* getName() override {
        return "Bluetooth";
    }
}; 