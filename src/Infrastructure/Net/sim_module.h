#pragma once
#include <Arduino.h>
#include <cstdint>
#include <string>
#include "TinyGsmClientBridge.h"

struct SimLbsSnapshot {
    bool     hasCellId      = false;
    bool     hasCoordinates = false;
    int      mcc            = 0;
    int      mnc            = 0;
    uint32_t lac            = 0;
    uint32_t cellId         = 0;
    double   latitude       = 0.0;
    double   longitude      = 0.0;
    int      accuracyM      = 0;
};

bool initSIM();
void probeSimProvisioningState();
bool isSimConnected();
int  getSignalRSSI();
bool isSimJammingDetected();
std::string getSimResolvedApn();
std::string getSimLocalIp();   // IP local del enlace GPRS/LTE ("0.0.0.0" si no hay)
uint16_t getSimModemRecoveries();
SimLbsSnapshot getSimLbsSnapshot(bool requestCoordinatesIfNoGps = false);

// Returns the WiFiClient-compatible bridge over TinyGsmClient
// Used by mqtt_client.cpp to pass into ESP32_MQTTv5::begin(WiFiClient&, ...)
TinyGsmClientBridge& getSimClient();
