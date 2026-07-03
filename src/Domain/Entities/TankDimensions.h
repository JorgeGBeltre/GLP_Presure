#pragma once

namespace glp::domain {

/**
 * Dimensiones internas del tanque cilíndrico horizontal, en milímetros.
 * El dominio trabaja en mm; la conversión desde cm (como se guarda en config)
 * se hace en el borde.
 */
struct TankDimensions {
    float radiusMm = 0.0f;
    float lengthMm = 0.0f;

    static TankDimensions fromCm(float radiusCm, float lengthCm) {
        return TankDimensions{radiusCm * 10.0f, lengthCm * 10.0f};
    }
};

} // namespace glp::domain
