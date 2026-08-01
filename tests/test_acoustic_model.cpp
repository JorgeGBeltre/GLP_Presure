#include <gtest/gtest.h>

#include "Domain/Services/AcousticModel.h"

using namespace glp::domain;

TEST(AcousticModel, GasModelWhenNoReflector) {
    UltrasonicRaw us;
    us.echoValid = true;
    us.tFlightUs = 800.0f;
    us.refValid  = false;

    const AcousticEstimate e = AcousticModel::estimate(us, 25.0f, 1.0f, 0.0f);
    EXPECT_FALSE(e.reflector);
    EXPECT_NEAR(e.cMs, LpgThermo::soundSpeedVaporMs(25.0f, 1.0f), 1e-3f);
    EXPECT_NEAR(e.cMs, 225.0f, 6.0f);       // propano saturado ~225 m/s, no 343 (aire)
    EXPECT_GT(e.sigmaMs, 0.0f);
    EXPECT_TRUE(e.valid);
    EXPECT_NEAR(e.distanceMm, e.cMs * 800.0f / 2000.0f, 1e-3f);
}

TEST(AcousticModel, ReflectorUsedWhenValidAndHealthy) {
    UltrasonicRaw us;
    us.echoValid = true;
    us.tFlightUs = 800.0f;
    us.refValid  = true;
    us.tRefUs    = 2000.0f * 100.0f / 225.0f; // reflector a 100mm consistente con ~225 m/s

    const AcousticEstimate e = AcousticModel::estimate(us, 25.0f, 1.0f, 100.0f);
    EXPECT_TRUE(e.reflector);
    EXPECT_NEAR(e.cMs, 225.0f, 1.0f);
    EXPECT_TRUE(e.healthy);
    EXPECT_LT(e.sigmaMs, 5.0f);              // reflector: menor incertidumbre que el gas
}

TEST(AcousticModel, UnhealthyWhenReflectorDisagreesWithGas) {
    UltrasonicRaw us;
    us.echoValid = true;
    us.tFlightUs = 800.0f;
    us.refValid  = true;
    us.tRefUs    = 2000.0f * 100.0f / 343.0f; // reflector implica 343 (aire) — no coincide con el gas

    const AcousticEstimate e = AcousticModel::estimate(us, 25.0f, 1.0f, 100.0f);
    EXPECT_TRUE(e.reflector);
    EXPECT_FALSE(e.healthy);
}

TEST(AcousticModel, InvalidEchoGivesZeroDistance) {
    UltrasonicRaw us;
    us.echoValid = false;

    const AcousticEstimate e = AcousticModel::estimate(us, 25.0f, 1.0f, 0.0f);
    EXPECT_FALSE(e.valid);
    EXPECT_FLOAT_EQ(e.distanceMm, 0.0f);
}
