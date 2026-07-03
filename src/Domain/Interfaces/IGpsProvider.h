#pragma once

#include "Domain/Entities/GpsFix.h"

namespace glp::domain {

/** Proveedor de posición GPS. */
class IGpsProvider {
public:
    virtual ~IGpsProvider() = default;

    virtual void begin() = 0;

    /** Procesa bytes NMEA pendientes (no bloqueante). Llamar cada loop. */
    virtual void feed() = 0;

    /** Última posición conocida. */
    virtual GpsFix location() = 0;
};

} // namespace glp::domain
