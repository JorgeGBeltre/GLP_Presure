#include "Infrastructure/Messaging/Mqttv5Transport.h"

#include <Arduino.h>

#include "Infrastructure/Messaging/mqtt_client.h"  // extern ESP32_MQTTv5 mqtt; initMQTT/connectMQTT/mqttLoop/isMQTTConnected

namespace glp::infra {

bool Mqttv5Transport::connect() {
    if (!begun_) {
        initMQTT();     // registra onConnect()->subscribe+initOTA y el sumidero onMessage
        begun_ = true;
    }
    connectMQTT();
    return isMQTTConnected();
}

void Mqttv5Transport::disconnect() {
    // El driver actual no expone desconexión explícita; la reconexión la maneja
    // ESP32_MQTTv5 (setAutoReconnect). Se deja como no-op.
}

bool Mqttv5Transport::isConnected() const {
    return isMQTTConnected();
}

bool Mqttv5Transport::publish(const std::string& topic, const std::string& payload) {
    // Firma observada en implementation_plan.md:
    //   mqtt.publish(String topic, String payload, int qos, bool retain [, props*])
    mqtt.publish(String(topic.c_str()), String(payload.c_str()), 1, false);
    return isMQTTConnected();
}

void Mqttv5Transport::subscribe(const std::string& topic) {
    mqtt.subscribe(String(topic.c_str()), 1);
}

void Mqttv5Transport::onMessage(MessageHandler handler) {
    handler_ = std::move(handler);
    // Conecta el sumidero del driver: convierte Arduino String -> std::string y
    // entrega el mensaje al handler de Application.
    setMqttMessageSink([this](const String& topic, const String& payload) {
        if (handler_) handler_(topic.c_str(), payload.c_str());
    });
}

void Mqttv5Transport::loop() {
    mqttLoop();
}

} // namespace glp::infra
