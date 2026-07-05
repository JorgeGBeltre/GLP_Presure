#pragma once

#include <string>
#include <cstdint>

#include "Domain/Entities/SensorCalibration.h"

namespace glp::domain {

/**
 * Configuración del dispositivo — entidad de dominio PURA.
 *
 * Reemplaza al antiguo `struct AppConfig` (que usaba Arduino `String` y estaba
 * acoplado a `Preferences`). Aquí no hay ninguna dependencia de hardware: se usa
 * `std::string`, de modo que esta entidad compila y se testea en host.
 *
 * La persistencia vive en Infrastructure (ver IConfigRepository).
 */
struct DeviceConfig {
    // Conectividad celular
    std::string apn;
    std::string apnUser;
    std::string apnPass;

    // Broker MQTT
    std::string mqttBroker;
    int         mqttPort = 1883;
    std::string mqttUser;
    std::string mqttPass;

    // Conectividad WiFi (uplink alternativo al celular)
    std::string wifiSsid;
    std::string wifiPass;

    // Identidad y geometría del tanque
    std::string deviceId;
    float       tankRadiusCm = 0.0f;
    float       tankLengthCm = 0.0f;

    // Geometría adicional para el nivel (h = H − d) y calibración de sensores
    float             tankHeightCm  = 0.0f;   // H: plano del sensor → piso interior
    float             mountOffsetCm = 0.0f;   // zona muerta/standoff no incluida en H
    SensorCalibration calibration{};
    float             propaneFraction = 1.0f;   // 1.0 = propano puro (mezcla propano/butano)
    float             sensorAxialCm   = 0.0f;    // posición del sensor a lo largo del eje (tilt)
    float             refDistanceCm   = 0.0f;    // distancia al reflector de referencia acústico

    // OTA
    std::string otaHmacKey;

    // Estado
    bool provisioned = false;
};

} // namespace glp::domain
