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

    // Geometría y calibración de sensores (Tier 0)
    std::optional<float> tankHeightCm;
    std::optional<float> mountOffsetCm;
    std::optional<float> pressSlope;
    std::optional<float> pressOffset;
    std::optional<float> tempRRef;
    std::optional<float> tempVExc;
    std::optional<float> tempOffsetC;
};

} // namespace glp::app
