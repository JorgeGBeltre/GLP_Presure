#include <gtest/gtest.h>

#include "Domain/Services/LpgThermo.h"

using namespace glp::domain;

TEST(LpgThermo, PropaneDensityFallsWithTemperature) {
    EXPECT_GT(LpgThermo::densityKgL(0.0f, 1.0f), LpgThermo::densityKgL(40.0f, 1.0f));
}

TEST(LpgThermo, InterpolatesBetweenNodes) {
    // 10°C, propano puro: entre 0.529 (0°C) y 0.501 (20°C) => 0.515.
    EXPECT_NEAR(LpgThermo::densityKgL(10.0f, 1.0f), 0.515f, 1e-3f);
}

TEST(LpgThermo, BlendsPropaneAndButaneByFraction) {
    const float p   = LpgThermo::densityKgL(20.0f, 1.0f); // propano
    const float b   = LpgThermo::densityKgL(20.0f, 0.0f); // butano
    const float mix = LpgThermo::densityKgL(20.0f, 0.5f);
    EXPECT_NEAR(mix, 0.5f * (p + b), 1e-4f);
}

TEST(LpgThermo, ClampsOutsideTableRange) {
    EXPECT_FLOAT_EQ(LpgThermo::densityKgL(-50.0f, 1.0f), 0.554f); // <= -20
    EXPECT_FLOAT_EQ(LpgThermo::densityKgL(100.0f, 1.0f), 0.467f); // >= 40
}

TEST(LpgThermo, MassIsDensityTimesVolume) {
    EXPECT_NEAR(LpgThermo::massKg(0.5f, 100.0f), 50.0f, 1e-4f);
}

TEST(LpgThermo, VcfIsMassOverDensityAt15AndConservesMass) {
    const float frac = 1.0f, mass = 50.0f;
    const float v15 = LpgThermo::volume15C(mass, frac);
    EXPECT_NEAR(v15 * LpgThermo::densityKgL(15.0f, frac), mass, 1e-3f);
}

TEST(LpgThermo, VaporPressurePropaneAt25CIsAbout9_8Bar) {
    EXPECT_NEAR(LpgThermo::vaporPressureBar(25.0f, 1.0f), 9.77f, 0.2f);
}

TEST(LpgThermo, VaporPressureRisesWithTemperature) {
    EXPECT_GT(LpgThermo::vaporPressureBar(40.0f, 1.0f), LpgThermo::vaporPressureBar(0.0f, 1.0f));
}

TEST(LpgThermo, ButaneVaporPressureLowerThanPropane) {
    EXPECT_LT(LpgThermo::vaporPressureBar(25.0f, 0.0f), LpgThermo::vaporPressureBar(25.0f, 1.0f));
}

TEST(LpgThermo, VaporPressureNoPoleNear24C) {
    // El set en Celsius tendría un polo ~24.4 °C; con NIST en Kelvin no debe diverger.
    EXPECT_LT(LpgThermo::vaporPressureBar(24.0f, 1.0f), 20.0f);
    EXPECT_GT(LpgThermo::vaporPressureBar(24.0f, 1.0f), 0.0f);
}
