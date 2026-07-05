#pragma once

#include <algorithm>
#include <cmath>

#include "Domain/Entities/SensorSample.h"
#include "Domain/Entities/VolumeEstimate.h"
#include "Domain/Services/LpgThermo.h"

namespace glp::domain {

/** Puntuación de salud (0-100) por sensor. */
struct SensorHealthScores {
    float ultrasonic  = 100.0f;
    float pressure    = 100.0f;
    float temperature = 100.0f;
};

/**
 * Diagnóstico de salud por sensor. PURO, testeable en host.
 * - Ultrasónico: lectura inválida ⇒ 0.
 * - Temperatura: inválida ⇒ 0; fuera del rango físico del GLP ⇒ degradada.
 * - Presión: residual contra la presión de vapor esperada Antoine(T).
 */
class SensorHealth {
public:
    static SensorHealthScores evaluate(const VolumeEstimate& est, const SensorSample& raw,
                                       float propaneFraction) {
        SensorHealthScores h;

        if (!raw.levelValid) h.ultrasonic = 0.0f;

        if (!raw.tempValid) h.temperature = 0.0f;
        else if (est.tempCelsius < -40.0f || est.tempCelsius > 60.0f) h.temperature = 50.0f;

        if (!raw.pressureValid) {
            h.pressure = 0.0f;
        } else {
            // Residual P medida vs P de vapor esperada. k=15 ⇒ ~6.7 bar anula la salud.
            const float pSat  = LpgThermo::vaporPressureBar(est.tempCelsius, propaneFraction);
            const float resid = std::fabs(est.pressureBar - pSat);
            h.pressure = std::clamp(100.0f - resid * 15.0f, 0.0f, 100.0f);
        }
        return h;
    }
};

} // namespace glp::domain
