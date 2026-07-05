#include <gtest/gtest.h>

#include "Domain/Services/SensorHealth.h"

using namespace glp::domain;

TEST(SensorHealth, AllHealthyAtEquilibrium) {
    SensorSample raw;                       // todos los canales válidos por default
    VolumeEstimate est;
    est.tempCelsius = 25.0f;
    est.pressureBar = LpgThermo::vaporPressureBar(25.0f, 1.0f); // == P_sat

    const auto h = SensorHealth::evaluate(est, raw, 1.0f);
    EXPECT_FLOAT_EQ(h.ultrasonic, 100.0f);
    EXPECT_FLOAT_EQ(h.temperature, 100.0f);
    EXPECT_NEAR(h.pressure, 100.0f, 0.1f);
}

TEST(SensorHealth, InvalidLevelZerosUltrasonic) {
    SensorSample raw;
    raw.levelValid = false;
    VolumeEstimate est;
    est.tempCelsius = 25.0f;
    est.pressureBar = LpgThermo::vaporPressureBar(25.0f, 1.0f);

    EXPECT_FLOAT_EQ(SensorHealth::evaluate(est, raw, 1.0f).ultrasonic, 0.0f);
}

TEST(SensorHealth, PressureResidualDegradesPressureHealth) {
    SensorSample raw;
    VolumeEstimate est;
    est.tempCelsius = 25.0f;
    est.pressureBar = LpgThermo::vaporPressureBar(25.0f, 1.0f) + 2.0f; // +2 bar de desvío

    const float p = SensorHealth::evaluate(est, raw, 1.0f).pressure;
    EXPECT_LT(p, 100.0f);
    EXPECT_GT(p, 0.0f);              // 2 bar ⇒ 100 − 30 = 70
    EXPECT_NEAR(p, 70.0f, 0.5f);
}

TEST(SensorHealth, TempOutOfRangeDegradesTemperature) {
    SensorSample raw;
    VolumeEstimate est;
    est.tempCelsius = 80.0f;
    est.pressureBar = 10.0f;

    EXPECT_FLOAT_EQ(SensorHealth::evaluate(est, raw, 1.0f).temperature, 50.0f);
}
