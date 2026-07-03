#pragma once
#include "Infrastructure/Messaging/mqtt_client.h"   // provides ESP32_MQTTv5 declaration

void initOTA();   // called from inside mqtt.onConnect()
void otaHandle(); // called every loop()
bool otaInProgress(); // true mientras hay una descarga/flasheo OTA en curso
