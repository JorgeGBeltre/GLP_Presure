#pragma once

#include <string>

namespace glp::domain {

/**
 * Posición GPS + timestamp + filtro de deriva estática (GTRDF) y guardia de
 * desplazamiento no autorizado del tanque estacionario (GTTOW / GTGEO).
 */
struct GpsFix {
    double      latitude      = 0.0;
    double      longitude     = 0.0;
    float       displacementM = 0.0f;
    bool        valid         = false;
    bool        driftFiltered = false;
    bool        tamperMoved   = false;
    std::string timestamp;       // ISO-8601 derivado del GPS, o "" si no hay fix
};

} // namespace glp::domain
