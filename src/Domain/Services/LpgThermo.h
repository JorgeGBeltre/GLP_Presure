#pragma once

#include <algorithm>

namespace glp::domain {

/**
 * Termodinámica del GLP: densidad por tabla (propano/butano), masa y volumen
 * corregido a 15 °C (VCF). PURA, testeable en host.
 *
 * Correcciones de diseño:
 *   - Densidad por tabla interpolada (no la recta 0.58−0.00125(T−15)).
 *   - masa = ρ(T)·V_medido, sin double-count de temperatura.
 *   - V15 (facturación) es una salida SEPARADA, no un eslabón hacia la masa.
 */
class LpgThermo {
public:
    // Densidad de líquido saturado (kg/L) por componente. Referencia NIST/GPA
    // (valores a confirmar contra fuente citada). Pendiente monotónica con T.
    static constexpr int   kN            = 4;
    static constexpr float kTemp[kN]     = {-20.0f, 0.0f, 20.0f, 40.0f};
    static constexpr float kPropane[kN]  = {0.554f, 0.529f, 0.501f, 0.467f};
    static constexpr float kButane[kN]   = {0.615f, 0.601f, 0.579f, 0.556f};

    /** Densidad del líquido (kg/L) a T (°C), mezcla por fracción de propano [0..1]. */
    static float densityKgL(float tempC, float propaneFraction) {
        const float frac = std::clamp(propaneFraction, 0.0f, 1.0f);
        const float rhoP = interp(tempC, kPropane);
        const float rhoB = interp(tempC, kButane);
        return frac * rhoP + (1.0f - frac) * rhoB;
    }

    /** Masa (kg) = densidad (kg/L) · volumen (L). */
    static float massKg(float densityKgL, float volumeLiters) {
        return densityKgL * volumeLiters;
    }

    /** Volumen corregido a 15 °C (L) para facturación: masa / ρ(15). Salida separada. */
    static float volume15C(float massKg, float propaneFraction) {
        const float rho15 = densityKgL(15.0f, propaneFraction);
        return (rho15 > 0.0f) ? massKg / rho15 : 0.0f;
    }

private:
    /** Interpolación lineal; fuera del rango de la tabla, clamp al extremo. */
    static float interp(float t, const float table[kN]) {
        if (t <= kTemp[0])      return table[0];
        if (t >= kTemp[kN - 1]) return table[kN - 1];
        for (int i = 1; i < kN; ++i) {
            if (t <= kTemp[i]) {
                const float f = (t - kTemp[i - 1]) / (kTemp[i] - kTemp[i - 1]);
                return table[i - 1] + f * (table[i] - table[i - 1]);
            }
        }
        return table[kN - 1];
    }
};

} // namespace glp::domain
