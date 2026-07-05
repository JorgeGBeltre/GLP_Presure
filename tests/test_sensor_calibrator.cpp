#include <gtest/gtest.h>

#include "Domain/Services/SensorCalibrator.h"

using namespace glp::domain;

TEST(SensorCalibrator, Pt1000MatchesIec60751ReferencePoints) {
    // R→T contra puntos tabulados IEC 60751 (±0.05 °C).
    EXPECT_NEAR(SensorCalibrator::pt1000ToCelsius(1000.000f),   0.0f, 0.05f);
    EXPECT_NEAR(SensorCalibrator::pt1000ToCelsius(1097.347f),  25.0f, 0.05f);
    EXPECT_NEAR(SensorCalibrator::pt1000ToCelsius(1385.055f), 100.0f, 0.05f);
    EXPECT_NEAR(SensorCalibrator::pt1000ToCelsius( 803.063f), -50.0f, 0.05f);
    EXPECT_NEAR(SensorCalibrator::pt1000ToCelsius(1758.560f), 200.0f, 0.05f);
}

TEST(SensorCalibrator, ResistanceFromVoltsInvertsLowArmDivider) {
    // Pt1000 en brazo bajo: Vpin = Vexc*Rpt/(Rpt+Rref). Con Rpt=1097.347, Rref=1000, Vexc=3.3:
    const float rpt = 1097.347f, rref = 1000.0f, vexc = 3.30f;
    const float vpin = vexc * rpt / (rpt + rref);
    EXPECT_NEAR(SensorCalibrator::resistanceFromVolts(vpin, rref, vexc), rpt, 0.05f);
}

TEST(SensorCalibrator, PressureTwoPointLinear) {
    // Dos puntos: (0.5V→0bar), (2.5V→40bar) ⇒ slope=20, offset=-10.
    const float slope = 20.0f, offset = -10.0f;
    EXPECT_NEAR(SensorCalibrator::voltsToBar(0.5f, slope, offset),  0.0f, 1e-3f);
    EXPECT_NEAR(SensorCalibrator::voltsToBar(2.5f, slope, offset), 40.0f, 1e-3f);
    EXPECT_NEAR(SensorCalibrator::voltsToBar(1.5f, slope, offset), 20.0f, 1e-3f);
}
