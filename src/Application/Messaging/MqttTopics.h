#pragma once

#include <string>

namespace glp::app {

/**
 * Tópicos MQTT derivados del device_id. Puro string-building, sin dependencias.
 * Coincide con la tabla de tópicos de Arquitectura_ESP32.md.
 */
struct MqttTopics {
    std::string telemetry;   // glp/telemetria/{id}
    std::string status;      // glp/status/{id}
    std::string config;      // glp/config/{id}
    std::string ota;         // ota/{id}
    std::string otaProgress; // ota/{id}/progress

    static MqttTopics forDevice(const std::string& id) {
        MqttTopics t;
        t.telemetry   = "glp/telemetria/" + id;
        t.status      = "glp/status/" + id;
        t.config      = "glp/config/" + id;
        t.ota         = "ota/" + id;
        t.otaProgress = "ota/" + id + "/progress";
        return t;
    }
};

} // namespace glp::app
