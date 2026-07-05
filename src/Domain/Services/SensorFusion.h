#pragma once

#include <algorithm>
#include <cmath>

#include "Domain/Entities/SensorSample.h"
#include "Domain/Entities/TankDimensions.h"
#include "Domain/Entities/VolumeEstimate.h"
#include "Domain/Services/KalmanFilter.h"
#include "Domain/Services/LpgThermo.h"

namespace glp::domain {

/**
 * Fusión de sensores para estimación de nivel de GLP — versión PURA.
 *
 * Migrada desde src/kalman/sensor_fusion.h. Cambios respecto al original:
 *   - `constrain(...)`  → std::clamp
 *   - `PI`              → constante local kPi
 *   - `acosf/sinf/sqrtf`→ <cmath>
 * Sin estos macros de Arduino, la clase compila y se testea en host.
 */
class SensorFusion {
public:
    SensorFusion()
        : kfLevel_(0.005f, 2.5f),   // Q=0.005mm², R=2.5mm²
          kfPressure_(0.01f, 0.5f)  // Q=0.01bar², R=0.5bar²
    {}

    /**
     * Procesa una nueva lectura de todos los sensores.
     * @param s    Muestra cruda (nivel mm, presión bar, temp °C)
     * @param tank Dimensiones internas del tanque (mm)
     */
    VolumeEstimate process(const SensorSample& s, const TankDimensions& tank,
                           float propaneFraction = 1.0f) {
        VolumeEstimate r;

        // 1. Nivel: Kalman si es válido; si no, sólo predicción (sostiene y crece σ).
        if (s.levelValid) r.levelMm = kfLevel_.update(s.levelRawMm);
        else { kfLevel_.predict(); r.levelMm = kfLevel_.estimate(); }
        r.kalmanUncertaintyMm = std::sqrt(kfLevel_.uncertainty());

        // 2. Presión: idem.
        if (s.pressureValid) r.pressureBar = kfPressure_.update(s.pressureRawBar);
        else { kfPressure_.predict(); r.pressureBar = kfPressure_.estimate(); }

        // 3. Temperatura (EMA): si es inválida, se sostiene el último valor.
        if (s.tempValid) {
            tempEma_ = uninit(tempEma_) ? s.tempRawC
                                        : 0.1f * s.tempRawC + 0.9f * tempEma_;
        }
        r.tempCelsius = uninit(tempEma_) ? s.tempRawC : tempEma_;

        // 4. Densidad del GLP por tabla interpolada (mezcla propano/butano)
        r.densityKgL = LpgThermo::densityKgL(r.tempCelsius, propaneFraction);

        // 5. Volumen del tanque cilíndrico horizontal (segmento circular)
        const float R = tank.radiusMm;
        if (R > 0.0f && tank.lengthMm > 0.0f) {
            const float h = std::clamp(r.levelMm, 0.0f, 2.0f * R);
            const float theta = 2.0f * std::acos((R - h) / R);
            const float areaSegment = R * R * (theta - std::sin(theta)) / 2.0f;
            r.volumeLiters = (areaSegment * tank.lengthMm) / 1'000'000.0f; // mm³ → L

            const float volumeTotal =
                (kPi * R * R * tank.lengthMm) / 1'000'000.0f;
            r.percentage = (volumeTotal > 0.0f)
                               ? (r.volumeLiters / volumeTotal) * 100.0f
                               : 0.0f;
        }

        // 6. Litros → galones (1 L = 0.264172 gal)
        r.gallons = r.volumeLiters * 0.264172f;

        // 6b. Masa y volumen corregido a 15 °C (facturación)
        r.massKg         = LpgThermo::massKg(r.densityKgL, r.volumeLiters);
        r.volume15Liters = LpgThermo::volume15C(r.massKg, propaneFraction);

        // 7. Detección de anomalía: coherencia nivel ↔ presión
        const bool highLevel   = (r.percentage > 5.0f);
        const bool lowPressure = (r.pressureBar < 1.0f);
        r.anomaly = (highLevel && lowPressure);

        return r;
    }

    /** Reinicia los filtros (llamar tras provisioning con nuevas dimensiones). */
    void reset() {
        kfLevel_.reset(0.0f, 100.0f);
        kfPressure_.reset(0.0f, 10.0f);
        tempEma_ = kUninit;
    }

private:
    static constexpr float kPi     = 3.14159265358979323846f;
    static constexpr float kUninit = -999.0f;
    static bool uninit(float v) { return v <= kUninit; }

    KalmanFilter kfLevel_;
    KalmanFilter kfPressure_;
    float        tempEma_ = kUninit; // -999 = no inicializado
};

} // namespace glp::domain
