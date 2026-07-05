#pragma once

namespace glp::domain {

/** Parámetros de calibración de los sensores analógicos, capturados en provisioning. */
struct SensorCalibration {
    // Presión TP12 (lineal, 2 puntos): bar = slope*V + offset
    float pressSlope  = 0.0f;
    float pressOffset = 0.0f;
    float pressMinBar = 0.0f;   // clamps de plausibilidad
    float pressMaxBar = 40.0f;
    // Temperatura Pt1000 (divisor brazo bajo + trim)
    float tempRRef    = 1000.0f; // Ω resistor de referencia
    float tempVExc    = 3.30f;   // V excitación del divisor
    float tempOffsetC = 0.0f;    // trim de 1 punto
};

} // namespace glp::domain
