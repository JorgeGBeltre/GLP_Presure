#pragma once
#include <Arduino.h>
#include <string>
#include "TinyGsmClientBridge.h"

bool initSIM();
bool isSimConnected();
int  getSignalRSSI();
std::string getSimLocalIp();   // IP local del enlace GPRS/LTE ("0.0.0.0" si no hay)

// Returns the WiFiClient-compatible bridge over TinyGsmClient
// Used by mqtt_client.cpp to pass into ESP32_MQTTv5::begin(WiFiClient&, ...)
TinyGsmClientBridge& getSimClient();
