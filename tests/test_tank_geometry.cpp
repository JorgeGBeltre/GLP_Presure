#include <gtest/gtest.h>

#include "Domain/Services/TankGeometry.h"

using namespace glp::domain;

TEST(TankGeometry, LevelIsHeightMinusDistanceMinusOffset) {
    // H=1000mm, offset=0, R=500 (2R=1000). d=1000 ⇒ h=0; d=0 ⇒ h=1000 (clamp 2R).
    EXPECT_FLOAT_EQ(TankGeometry::levelFromDistance(1000.0f, 1000.0f, 0.0f, 500.0f),   0.0f);
    EXPECT_FLOAT_EQ(TankGeometry::levelFromDistance(   0.0f, 1000.0f, 0.0f, 500.0f), 1000.0f);
    EXPECT_FLOAT_EQ(TankGeometry::levelFromDistance( 400.0f, 1000.0f, 0.0f, 500.0f),  600.0f);
}

TEST(TankGeometry, MountOffsetSubtractsDeadZone) {
    // offset=50 ⇒ h = H - d - 50.
    EXPECT_FLOAT_EQ(TankGeometry::levelFromDistance(400.0f, 1000.0f, 50.0f, 500.0f), 550.0f);
}

TEST(TankGeometry, ClampsToZeroAndDiameter) {
    EXPECT_FLOAT_EQ(TankGeometry::levelFromDistance(2000.0f, 1000.0f, 0.0f, 500.0f),    0.0f); // d>H
    EXPECT_FLOAT_EQ(TankGeometry::levelFromDistance(-500.0f, 1000.0f, 0.0f, 500.0f), 1000.0f); // >2R
}
