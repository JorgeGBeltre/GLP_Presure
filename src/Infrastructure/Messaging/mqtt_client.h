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
// Mqttv5Transport para entregar (topic, payload) a la capa de Application, en
// lugar de manejar la config remota aquí abajo.
void setMqttMessageSink(std::function<void(const String& topic, const String& payload)> sink);
