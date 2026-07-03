#pragma once

#include <optional>
#include <string>

namespace glp::app {

/**
 * Actualización de configuración recibida por MQTT (tópico glp/config/{id}).
 * El parseo del JSON vive en Infrastructure; la Application recibe este struct
 * ya tipado. Sólo los campos presentes se aplican.
 */
struct RemoteConfigUpdate {
    std::optional<float>       tankRadiusCm;
    std::optional<float>       tankLengthCm;
    std::optional<std::string> mqttBroker;
    std::optional<int>         mqttPort;
    std::optional<std::string> otaHmacKey;
};

} // namespace glp::app
