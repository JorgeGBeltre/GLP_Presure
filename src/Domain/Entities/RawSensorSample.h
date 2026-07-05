#pragma once

namespace glp::domain {

/** Lectura CRUDA del hardware, agnóstica del ADC. Producida por un IRawSensorReader. */
struct RawSensorSample {
    float echoDistanceMm = 0.0f;   // distancia por tiempo de vuelo (c del aire hasta Tier 3)
    bool  echoValid      = false;  // false si timeout / fuera de rango
    float pressureVolts  = 0.0f;   // tensión en el pin (cuentas→V ya normalizado por el reader)
    bool  pressureValid  = false;
    float tempVolts      = 0.0f;   // tensión del divisor Pt1000
    bool  tempValid      = false;
};

} // namespace glp::domain
