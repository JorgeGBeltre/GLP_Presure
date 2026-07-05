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
        StaticJsonDocument<768> doc;
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
        if (doc.containsKey("tank_height_cm"))  u.tankHeightCm  = doc["tank_height_cm"].as<float>();
        if (doc.containsKey("mount_offset_cm")) u.mountOffsetCm = doc["mount_offset_cm"].as<float>();
        if (doc.containsKey("press_slope"))     u.pressSlope    = doc["press_slope"].as<float>();
        if (doc.containsKey("press_offset"))    u.pressOffset   = doc["press_offset"].as<float>();
        if (doc.containsKey("temp_r_ref"))      u.tempRRef      = doc["temp_r_ref"].as<float>();
        if (doc.containsKey("temp_v_exc"))      u.tempVExc      = doc["temp_v_exc"].as<float>();
        if (doc.containsKey("temp_offset_c"))   u.tempOffsetC   = doc["temp_offset_c"].as<float>();
        if (doc.containsKey("propane_fraction")) u.propaneFraction = doc["propane_fraction"].as<float>();
        if (doc.containsKey("sensor_axial_cm")) u.sensorAxialCm  = doc["sensor_axial_cm"].as<float>();
        return u;
    }
};

} // namespace glp::infra
