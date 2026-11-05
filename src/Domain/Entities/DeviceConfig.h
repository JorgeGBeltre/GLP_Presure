#pragma once

#include <string>
#include "Domain/Entities/SensorCalibration.h"

namespace glp::domain {

/**
 * Configuración persistente del dispositivo (entidad pura; sin dependencias de
 * Preferences/NVS ni de Arduino String). Incluye identidad de hardware (`deviceKey`),
 * token de vinculación (`bindToken`) y control del indicador LED (`sledOnline`).
 */
struct DeviceConfig {
    // Conectividad celular
    std::string apn;
    std::string apnUser;
    std::string apnPass;

    // Broker MQTT
    std::string mqttBroker;
    int         mqttPort = 8883;
    std::string mqttUser;
    std::string mqttPass;

    // WiFi
    std::string wifiSsid;
    std::string wifiPass;

    // Identidad y vinculación
    std::string deviceId;
    std::string deviceKey;
    std::string bindToken;

    // Geometría del tanque (cilindro horizontal con cabezas hemisféricas)
    float tankRadiusCm = 0.0f;
    float tankLengthCm = 0.0f;
    float tankHeightCm = 0.0f;
    float mountOffsetCm = 0.0f;

    // Calibración de transductores
    SensorCalibration calibration{};

    // Composición y montaje ultrasónico bottom-up
    float propaneFraction = 0.60f;
    float sensorAxialCm   = 0.0f;
    float refDistanceCm   = 0.0f;

    // Seguridad OTA y estado visual
    std::string otaHmacKey;
    bool        sledOnline   = true;
    bool        accountBound = false;

    bool provisioned = false;
};

} // namespace glp::domain
