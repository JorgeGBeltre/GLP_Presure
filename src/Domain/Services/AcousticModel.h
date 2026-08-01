#pragma once

#include <cmath>

#include "Domain/Entities/AcousticEstimate.h"
#include "Domain/Entities/UltrasonicRaw.h"
#include "Domain/Services/LpgThermo.h"

namespace glp::domain {

/**
 * Modelo acústico del espacio de vapor. PURO, testeable en host.
 *
 * Combina tres fuentes para estimar la velocidad del sonido `c ± σ`:
 *   1. Reflector de referencia (primario cuando está sano): c = 2·D_ref / t_ref.
 *   2. Modelo de gas real (respaldo): c = √(γ·Z·R·T/M) según composición.
 *   3. Monitor de salud: compara ambas y marca desviaciones.
 * Con esa `c` calcula la distancia al líquido, que alimenta el nivel (y el EKF).
 */
class AcousticModel {
public:
    static AcousticEstimate estimate(const UltrasonicRaw& us, float tempC,
                                     float propaneFraction, float refDistanceMm) {
        AcousticEstimate e;

        const float cGas = LpgThermo::soundSpeedVaporMs(tempC, propaneFraction);

        if (us.refValid && refDistanceMm > 0.0f && us.tRefUs > 0.0f) {
            const float cRef = 2000.0f * refDistanceMm / us.tRefUs;    // mm/µs → m/s
            e.cMs       = cRef;               // el reflector es medición directa: prioridad
            e.sigmaMs   = 3.0f;               // baja incertidumbre
            e.reflector = true;
            e.healthy   = std::fabs(cRef - cGas) < 0.15f * cGas;       // tolerancia ancha
        } else {
            e.cMs       = cGas;
            e.sigmaMs   = 0.07f * cGas;       // ~7 %: composición + gas-real inciertos
            e.reflector = false;
            e.healthy   = true;               // sin reflector no hay con qué comparar
        }

        e.valid      = us.echoValid;
        e.distanceMm = us.echoValid ? e.cMs * us.tFlightUs / 2000.0f : 0.0f;
        return e;
    }
};

} // namespace glp::domain
