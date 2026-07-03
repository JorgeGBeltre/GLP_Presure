#pragma once

#include "Domain/Interfaces/ITelemetryTransport.h"

namespace glp::infra {

/**
 * ITelemetryTransport sobre ESP32_MQTTv5. Delega el ciclo de vida en las
 * funciones existentes de mqtt/mqtt_client (initMQTT/connectMQTT/mqttLoop/
 * isMQTTConnected) y publica genéricamente sobre la instancia global `mqtt`.
 */
class Mqttv5Transport : public domain::ITelemetryTransport {
public:
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    bool publish(const std::string& topic, const std::string& payload) override;
    void subscribe(const std::string& topic) override;
    void onMessage(MessageHandler handler) override;
    void loop() override;

private:
    bool           begun_ = false;
    MessageHandler handler_;
};

} // namespace glp::infra
