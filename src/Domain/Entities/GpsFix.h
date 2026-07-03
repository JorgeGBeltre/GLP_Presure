#pragma once

#include <string>

namespace glp::domain {

/**
 * Posición GPS + timestamp. Si `valid` es false, lat/lng no son confiables.
 */
struct GpsFix {
    double      latitude  = 0.0;
    double      longitude = 0.0;
    bool        valid     = false;
    std::string timestamp;       // ISO-8601 derivado del GPS, o "" si no hay fix
};

} // namespace glp::domain
