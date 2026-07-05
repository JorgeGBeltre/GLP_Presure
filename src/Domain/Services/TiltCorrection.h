#pragma once

#include <algorithm>
#include <cmath>

namespace glp::domain {

/**
 * Corrección geométrica por inclinación del tanque. PURA, testeable en host.
 *
 * Correcciones respecto a la aproximación ingenua Δh = R·sinθ (que la verificación
 * demostró incorrecta):
 *   - Sesgo axial: el nivel bajo el sensor (a x_s del centro) NO es el nivel medio;
 *     hay que restar x_s·tan(pitch).
 *   - Volumen: con la superficie inclinada, el volumen es la INTEGRAL del área de
 *     segmento a lo largo del eje, no A(h)·L.
 */
class TiltCorrection {
public:
    /** Nivel en el centro axial a partir del nivel medido en la estación del sensor. */
    static float levelAtCenter(float levelStationMm, float pitchRad, float sensorAxialMm) {
        return levelStationMm - sensorAxialMm * std::tan(pitchRad);
    }

    /**
     * Volumen (L) con superficie inclinada por `pitch`, integrando el área de
     * segmento a lo largo del eje (cuadratura midpoint de `nodes` nodos).
     * `levelCenterMm` es el nivel en el centro axial (ya de-sesgado).
     */
    static float tiltedVolumeLiters(float levelCenterMm, float pitchRad,
                                    float radiusMm, float lengthMm, int nodes = 24) {
        if (radiusMm <= 0.0f || lengthMm <= 0.0f) return 0.0f;
        const float slope = std::tan(pitchRad);         // mm de nivel por mm axial
        const float dx    = lengthMm / static_cast<float>(nodes);
        double acc = 0.0;
        for (int i = 0; i < nodes; ++i) {
            const float x = -lengthMm * 0.5f + (static_cast<float>(i) + 0.5f) * dx;
            const float h = std::clamp(levelCenterMm + x * slope, 0.0f, 2.0f * radiusMm);
            acc += segmentAreaMm2(h, radiusMm);
        }
        return static_cast<float>(acc * dx / 1'000'000.0); // mm³ → L
    }

private:
    static float segmentAreaMm2(float h, float R) {
        const float hh    = std::clamp(h, 0.0f, 2.0f * R);
        const float theta = 2.0f * std::acos((R - hh) / R);
        return R * R * (theta - std::sin(theta)) / 2.0f;
    }
};

} // namespace glp::domain
