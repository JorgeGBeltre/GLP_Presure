#pragma once

#include <cmath>

namespace glp::domain {

/** Conversión cruda→física de los sensores analógicos. PURA, testeable en host. */
class SensorCalibrator {
public:
    // Constantes Pt1000 IEC 60751
    static constexpr float kA  = 3.9083e-3f;
    static constexpr float kB  = -5.775e-7f;
    static constexpr float kC  = -4.183e-12f;   // sólo T < 0
    static constexpr float kR0 = 1000.0f;

    /** R(T) directa (Ω). */
    static float resistanceAt(float t) {
        if (t >= 0.0f) return kR0 * (1.0f + kA * t + kB * t * t);
        return kR0 * (1.0f + kA * t + kB * t * t + kC * (t - 100.0f) * t * t * t);
    }

    /** R (Ω) → °C. Forma cerrada (rama T≥0) como semilla + 3 Newton sobre la CVD completa. */
    static float pt1000ToCelsius(float r) {
        const float disc = kA * kA - 4.0f * kB * (1.0f - r / kR0);
        float t = (-kA + std::sqrt(disc)) / (2.0f * kB);   // semilla (negativa si r<R0)
        for (int i = 0; i < 3; ++i) {
            const float rt   = resistanceAt(t);
            const float drdt = (t >= 0.0f)
                ? kR0 * (kA + 2.0f * kB * t)
                : kR0 * (kA + 2.0f * kB * t + kC * (4.0f * t * t * t - 300.0f * t * t));
            t -= (rt - r) / drdt;
        }
        return t;
    }

    /** Divisor Pt1000 en brazo bajo: Vpin sobre Pt1000, Rref a Vexc. Devuelve R_pt (Ω). */
    static float resistanceFromVolts(float vPin, float rRef, float vExc) {
        return rRef * vPin / (vExc - vPin);
    }

    /** Presión lineal de 2 puntos: bar = slope*V + offset. */
    static float voltsToBar(float v, float slope, float offset) {
        return slope * v + offset;
    }
};

} // namespace glp::domain
