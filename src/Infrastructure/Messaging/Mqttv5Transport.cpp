#include "Infrastructure/Messaging/Mqttv5Transport.h"

#include <Arduino.h>

#include "Infrastructure/Messaging/mqtt_client.h"
#include "Infrastructure/Ota/ota_manager.h"

namespace glp::infra {

void Mqttv5Transport::enqueue(const std::string& topic, const std::string& payload) {
    if (topic.empty() || payload.empty() || payload.size() > kMaxPayloadBytes) {
        return;
    }
    const size_t slot = (head_ + count_) % kQueueCapacity;
    if (count_ == kQueueCapacity) {
        head_ = (head_ + 1) % kQueueCapacity;
    } else {
        ++count_;
    }
    queue_[slot].topic   = topic;
    queue_[slot].payload = payload;
}

size_t Mqttv5Transport::flushQueue(size_t maxPerTick) {
    if (!isMQTTConnected() || otaInProgress() || count_ == 0) return 0;
    size_t sent = 0;
    while (count_ > 0 && sent < maxPerTick && isMQTTConnected() && !otaInProgress()) {
        const QueuedFrame& item = queue_[head_];
        if (!mqtt.publish(String(item.topic.c_str()), String(item.payload.c_str()), 1, false)) {
            break;
        }
        queue_[head_].topic.clear();
        queue_[head_].payload.clear();
        head_ = (head_ + 1) % kQueueCapacity;
        --count_;
        ++sent;
    }
    return sent;
}

bool Mqttv5Transport::connect() {
    if (!begun_) {
        initMQTT();     // registra onConnect()->subscribe+initOTA y el sumidero onMessage
        begun_ = true;
    }
    connectMQTT();
    return isMQTTConnected();
}

void Mqttv5Transport::disconnect() {
    mqtt.disconnect();
}

bool Mqttv5Transport::isConnected() const {
    return isMQTTConnected();
}

bool Mqttv5Transport::publish(const std::string& topic, const std::string& payload) {
    if (!isMQTTConnected() || otaInProgress()) {
        if (!otaInProgress()) {
            enqueue(topic, payload);
        }
        return false;
    }
    if (!mqtt.publish(String(topic.c_str()), String(payload.c_str()), 1, false)) {
        enqueue(topic, payload);
        return false;
    }
    return true;
}

void Mqttv5Transport::subscribe(const std::string& topic) {
    mqtt.subscribe(String(topic.c_str()), 1);
}

void Mqttv5Transport::onMessage(MessageHandler handler) {
    handler_ = std::move(handler);
    setMqttMessageSink([this](const String& topic, const String& payload) {
        if (handler_) handler_(topic.c_str(), payload.c_str());
    });
}

void Mqttv5Transport::loop() {
    mqttLoop();
    flushQueue(2);
}

} // namespace glp::infra
