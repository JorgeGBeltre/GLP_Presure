#include <gtest/gtest.h>

#include <cmath>

#include "Domain/Services/TiltCorrection.h"

using namespace glp::domain;

static constexpr float kPi = 3.14159265358979f;
static float deg2rad(float d) { return d * kPi / 180.0f; }

TEST(TiltCorrection, AxialBiasRemovedRecoversCenterLevel) {
    // Sensor a +1600mm; con pitch=10° la estación lee alto; levelAtCenter lo corrige.
    const float pitch      = deg2rad(10.0f);
    const float trueCenter = 500.0f;
    const float station    = trueCenter + 1600.0f * std::tan(pitch);
    EXPECT_NEAR(TiltCorrection::levelAtCenter(station, pitch, 1600.0f), trueCenter, 0.5f);
}

TEST(TiltCorrection, ZeroPitchMatchesUprightHalfVolume) {
    // R=500, L=1000, nivel=R (medio lleno) => π·R²/2·L = 392.7 L.
    const float v = TiltCorrection::tiltedVolumeLiters(500.0f, 0.0f, 500.0f, 1000.0f);
    EXPECT_NEAR(v, 392.70f, 1.0f);
}

TEST(TiltCorrection, VolumeNearlyInvariantAtHalfFull) {
    // A medio tanque la inclinación casi no cambia el volumen (verificado por simulación).
    const float v0  = TiltCorrection::tiltedVolumeLiters(500.0f, 0.0f, 500.0f, 1000.0f);
    const float v10 = TiltCorrection::tiltedVolumeLiters(500.0f, deg2rad(10.0f), 500.0f, 1000.0f);
    EXPECT_NEAR(v10, v0, v0 * 0.02f); // < 2%
}

TEST(TiltCorrection, ZeroSensorOffsetLeavesLevelUnchanged) {
    EXPECT_FLOAT_EQ(TiltCorrection::levelAtCenter(600.0f, deg2rad(15.0f), 0.0f), 600.0f);
}
