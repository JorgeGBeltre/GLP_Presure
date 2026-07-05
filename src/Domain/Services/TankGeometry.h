#pragma once

#include <algorithm>

namespace glp::domain {

/** Geometría del tanque. PURA. Datum: H = del plano acústico del sensor al piso interior. */
class TankGeometry {
public:
    /** Nivel de líquido (mm) desde la distancia medida. h = H − d − mountOffset, clamp [0, 2R]. */
    static float levelFromDistance(float distanceMm, float heightMm,
                                   float mountOffsetMm, float radiusMm) {
        const float h = heightMm - distanceMm - mountOffsetMm;
        return std::clamp(h, 0.0f, 2.0f * radiusMm);
    }
};

} // namespace glp::domain
