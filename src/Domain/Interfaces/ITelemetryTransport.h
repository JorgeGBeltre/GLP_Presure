#pragma once

#include <functional>
#include <string>

namespace glp::domain {

/**
 * Transporte de mensajería (MQTT 5.0 en el firmware real).
 * La Application depende de esta interfaz, no de ESP32_MQTTv5.
 */
class ITelemetryTransport {
public:
    using MessageHandler =
        std::function<void(const std::string& topic, const std::string& payload)>;

    virtual ~ITelemetryTransport() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual bool publish(const std::string& topic, const std::string& payload) = 0;
    virtual void subscribe(const std::string& topic) = 0;
    virtual void onMessage(MessageHandler handler) = 0;

    /** Bombea el cliente MQTT (no bloqueante). Llamar cada loop. */
    virtual void loop() = 0;
};

} // namespace glp::domain
