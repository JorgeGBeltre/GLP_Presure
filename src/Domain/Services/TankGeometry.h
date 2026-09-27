#pragma once

#include <algorithm>

namespace glp::domain {

/** Geometría del tanque. PURA. Datum: H = del plano acústico del sensor al piso interior. */
class TankGeometry {
public:
    /** Nivel de líquido (mm) desde la distancia medida. h = H − d − mountOffset, clamp [0, 2R].
     * Si heightMm <= 0 (no configurado explícitamente), se asume la altura interna nominal 2R. */
    static float levelFromDistance(float distanceMm, float heightMm,
                                   float mountOffsetMm, float radiusMm) {
        const float effectiveH = (heightMm > 0.0f) ? heightMm : (2.0f * radiusMm);
        const float h = effectiveH - distanceMm - mountOffsetMm;
        return std::clamp(h, 0.0f, 2.0f * radiusMm);
    }
};

} // namespace glp::domain
