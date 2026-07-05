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
    float soundSpeedMs    = 343.0f; // c estimada usada para la distancia
    bool  reflectorUsed   = false;  // el reflector de referencia fue la fuente de c
    bool  acousticHealthy = true;   // reflector y modelo de gas coinciden
};

} // namespace glp::domain
