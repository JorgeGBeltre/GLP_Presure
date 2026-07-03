#pragma once

#include "Domain/Entities/DeviceConfig.h"

namespace glp::domain {

/** Persistencia de la configuración del dispositivo (NVS en el ESP32). */
class IConfigRepository {
public:
    virtual ~IConfigRepository() = default;

    /** Carga la config; devuelve false si no hay nada guardado. */
    virtual bool load(DeviceConfig& out) = 0;

    /** Guarda la config completa. */
    virtual bool save(const DeviceConfig& cfg) = 0;

    /** Borra toda la config (fábrica). */
    virtual bool erase() = 0;

    /** true si el dispositivo ya completó el provisioning. */
    virtual bool isProvisioned() = 0;
};

} // namespace glp::domain
