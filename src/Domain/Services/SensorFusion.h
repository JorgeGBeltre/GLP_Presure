#pragma once

#include <algorithm>
#include <cmath>

#include "Domain/Entities/SensorSample.h"
#include "Domain/Entities/TankDimensions.h"
#include "Domain/Entities/VolumeEstimate.h"
#include "Domain/Services/KalmanFilter.h"
#include "Domain/Services/LpgThermo.h"
#include "Domain/Services/StateEkf.h"
#include "Domain/Services/TiltCorrection.h"
#include "Domain/Entities/TiltReading.h"

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
        // EKF [h,T]: qH,qT (proceso); rH (ultrasónico); rT (Pt1000); rTfromP (T desde presión)
        : ekf_(0.005f, 0.01f, 2.5f, 0.25f, 1.0f),
          kfPressure_(0.01f, 0.5f)  // filtro de presión: sólo para reporte + inversa de Antoine
    {}

    /**
     * Procesa una nueva lectura de todos los sensores.
     * @param s    Muestra cruda (nivel mm, presión bar, temp °C)
     * @param tank Dimensiones internas del tanque (mm)
     */
    VolumeEstimate process(const SensorSample& s, const TankDimensions& tank,
                           float propaneFraction = 1.0f,
                           TiltReading tilt = {}, float sensorAxialMm = 0.0f) {
        VolumeEstimate r;

        // 1. Presión filtrada (para reporte y para la inversa de Antoine).
        if (s.pressureValid) r.pressureBar = kfPressure_.update(s.pressureRawBar);
        else { kfPressure_.predict(); r.pressureBar = kfPressure_.estimate(); }

        // 2. EKF [h, T] (opción B): nivel del ultrasónico, temperatura del Pt1000 y una
        //    2ª medición de temperatura por Antoine⁻¹(P). Un canal inválido ⇒ ese estado
        //    sólo predice (sostiene el valor y crece su σ).
        const bool  tFromPValid = s.pressureValid && r.pressureBar > 0.5f;
        const float tFromP = tFromPValid
            ? LpgThermo::temperatureFromVaporPressure(r.pressureBar, propaneFraction)
            : 0.0f;
        ekf_.update(s.levelRawMm, s.levelValid, s.tempRawC, s.tempValid, tFromP, tFromPValid);
        r.levelMm             = ekf_.level();
        r.tempCelsius         = ekf_.temp();
        r.kalmanUncertaintyMm = std::sqrt(ekf_.levelVar());

        // 4. Densidad del GLP por tabla interpolada (mezcla propano/butano)
        r.densityKgL = LpgThermo::densityKgL(r.tempCelsius, propaneFraction);

        // 5. Corrección por inclinación (de-sesgo axial) + volumen integrando el plano
        //    inclinado a lo largo del eje. Sin inclinación válida ⇒ pitch=0 y la
        //    integral coincide con el segmento recto A(h)·L.
        const float pitch = tilt.valid ? tilt.pitchRad : 0.0f;
        r.levelMm = TiltCorrection::levelAtCenter(r.levelMm, pitch, sensorAxialMm);
        const float R = tank.radiusMm;
        if (R > 0.0f && tank.lengthMm > 0.0f) {
            r.volumeLiters = TiltCorrection::tiltedVolumeLiters(r.levelMm, pitch, R, tank.lengthMm);
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
        ekf_.reset(0.0f, 15.0f, 100.0f);
        kfPressure_.reset(0.0f, 10.0f);
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;

    StateEkf     ekf_;
    KalmanFilter kfPressure_;
};

} // namespace glp::domain
