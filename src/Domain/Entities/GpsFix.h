#pragma once

#include <cstdint>
#include <string>

namespace glp::domain {

/**
 * Posición GPS + timestamp + filtro de deriva estática (GTRDF), posicionamiento
 * celular LBS (Cell ID + coordenadas de estación base) y guardia de
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

    // LBS (Location-Based Service / Cell ID positioning)
    bool        lbsValid      = false;
    bool        isLbsFallback = false;
    int         mcc           = 0;
    int         mnc           = 0;
    uint32_t    lac           = 0;
    uint32_t    cellId        = 0;
    int         lbsAccuracyM  = 0;
};

} // namespace glp::domain
