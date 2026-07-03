#pragma once

#include <string>

namespace glp::domain {

/**
 * Identidad del dispositivo para la cabecera "Device" de los mensajes.
 * (Patrón tomado del proyecto Toour_IoT.) Puro: std::string, sin Arduino.
 */
struct DeviceIdentity {
    std::string chipId;          // derivado del eFuse MAC
    std::string chipType;        // ESP.getChipModel() (ej. "ESP32-S3")
    std::string macAddress;      // MAC WiFi STA
    std::string firmwareVersion;
    std::string deviceId;        // id de negocio (ej. "GLP-001", de config)
    std::string imei;            // del módem SIM
    std::string imsi;
    std::string simOperator;
    int         signalQuality = 0;
};

} // namespace glp::domain
