#pragma once

#include <string>

#include "Domain/Entities/DeviceIdentity.h"

namespace glp::app {

/**
 * Cabecera común a todos los mensajes salientes (bloque "Device" + "Timestamp").
 * Patrón tomado de Toour_IoT: cada mensaje sólo rellena su "Details".
 */
struct EnvelopeContext {
    domain::DeviceIdentity identity;
    std::string            deviceId;        // id de negocio (config)
    std::string            connectionType;  // "Wifi" | "SimCard"
    std::string            ipAddress;
    std::string            timestamp;        // ISO-8601
};

} // namespace glp::app
