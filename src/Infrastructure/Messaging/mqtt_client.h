#pragma once
#include <functional>
#include "ESP32_MQTTv5.h"

// Global MQTT instance — accessible by ota_manager.cpp via extern
extern ESP32_MQTTv5 mqtt;

void initMQTT();
void connectMQTT();
void publishTelemetry(const String& jsonPayload);
void mqttLoop();
bool isMQTTConnected();

// Sumidero de mensajes entrantes. Lo registra Infrastructure/Messaging/
// Mqttv5Transport para entregar (topic, payload) a la capa de Application.
void setMqttMessageSink(std::function<void(const String& topic, const String& payload)> sink);

// Callback invocado cuando se verifica criptográficamente una orden de desvinculación de cuenta.
void setMqttUnlinkVerifiedCallback(std::function<void()> cb);
