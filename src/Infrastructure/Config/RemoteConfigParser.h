#pragma once

#include <optional>
#include <string>

#include <ArduinoJson.h>

#include "Application/Dto/RemoteConfigUpdate.h"

namespace glp::infra {

/**
 * Parsea el JSON recibido en glp/config/{id} a un RemoteConfigUpdate tipado.
 * El parseo (ArduinoJson) es un detalle de Infrastructure; la Application sólo
 * ve el struct con optionals. Devuelve nullopt si el JSON es inválido.
 */
class RemoteConfigParser {
public:
    static std::optional<app::RemoteConfigUpdate> parse(const std::string& json) {
        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, json)) return std::nullopt;

        app::RemoteConfigUpdate u;
        if (doc.containsKey("tank_radius_cm")) u.tankRadiusCm = doc["tank_radius_cm"].as<float>();
        if (doc.containsKey("tank_length_cm")) u.tankLengthCm = doc["tank_length_cm"].as<float>();
        if (doc.containsKey("mqtt_port"))      u.mqttPort     = doc["mqtt_port"].as<int>();
        if (doc.containsKey("mqtt_broker")) {
            const char* s = doc["mqtt_broker"];
            if (s) u.mqttBroker = s;
        }
        if (doc.containsKey("ota_hmac_key")) {
            const char* s = doc["ota_hmac_key"];
            if (s) u.otaHmacKey = s;
        }
        return u;
    }
};

} // namespace glp::infra
