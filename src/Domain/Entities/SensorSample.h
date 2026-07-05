#pragma once

namespace glp::domain {

/**
 * Lectura CRUDA de los tres sensores físicos, antes de cualquier filtrado.
 * Producida por un ISensorReader (Infrastructure).
 */
struct SensorSample {
    float levelRawMm     = 0.0f;  // ultrasónico
    float pressureRawBar = 0.0f;  // sensor de presión
    float tempRawC       = 0.0f;  // sensor de temperatura
    bool  levelValid     = true;  // false si la lectura del canal es inválida (timeout/riel)
    bool  pressureValid  = true;
    bool  tempValid      = true;
};

} // namespace glp::domain
