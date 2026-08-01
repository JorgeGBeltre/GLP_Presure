#pragma once

#include "Domain/Entities/UltrasonicRaw.h"

namespace glp::domain {

/**
 * Lectura CRUDA del hardware, agnóstica del ADC y sin conversiones físicas: el
 * ultrasónico entrega tiempos (UltrasonicRaw), no distancia. La velocidad del
 * sonido y la distancia se calculan en el dominio (AcousticModel).
 */
struct RawSensorSample {
    UltrasonicRaw ultrasonic;      // tiempos de vuelo + calidad
    float pressureVolts  = 0.0f;   // tensión en el pin (cuentas→V ya normalizado)
    bool  pressureValid  = false;
    float tempVolts      = 0.0f;   // tensión del divisor Pt1000
    bool  tempValid      = false;
};

} // namespace glp::domain
