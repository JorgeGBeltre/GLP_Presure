#include <gtest/gtest.h>

#include "Domain/Services/SensorFusion.h"

using namespace glp::domain;

namespace {
VolumeEstimate feedManyTimes(SensorFusion& f, const SensorSample& s,
                             const TankDimensions& tank, int n) {
    VolumeEstimate r;
    for (int i = 0; i < n; ++i) r = f.process(s, tank);
    return r;
}
} // namespace

TEST(SensorFusion, HalfFullHorizontalCylinderIsFiftyPercent) {
    SensorFusion f;
    f.reset();
    // Tanque R=500mm, L=1000mm. Nivel = R => medio lleno => 50% exacto.
    const TankDimensions tank{500.0f, 1000.0f};
    const SensorSample sample{/*level*/ 500.0f, /*pressure*/ 6.0f, /*temp*/ 15.0f};

    const VolumeEstimate r = feedManyTimes(f, sample, tank, 300);

    EXPECT_NEAR(r.levelMm, 500.0f, 5.0f);
    EXPECT_NEAR(r.percentage, 50.0f, 2.0f);
    EXPECT_GT(r.gallons, 0.0f);
    EXPECT_FALSE(r.anomaly);
    EXPECT_LT(r.kalmanUncertaintyMm, 1.0f); // el filtro convergió
}

TEST(SensorFusion, DensityDropsWithTemperature) {
    SensorFusion f;
    f.reset();
    const TankDimensions tank{500.0f, 1000.0f};
    // A 15°C la densidad base es 0.58; a mayor temperatura, menor densidad.
    const VolumeEstimate cold = feedManyTimes(f, {500.0f, 6.0f, 15.0f}, tank, 300);

    SensorFusion g;
    g.reset();
    const VolumeEstimate hot = feedManyTimes(g, {500.0f, 6.0f, 35.0f}, tank, 300);

    EXPECT_GT(cold.densityKgL, hot.densityKgL);
}

TEST(SensorFusion, AnomalyWhenHighLevelButLowPressure) {
    SensorFusion f;
    f.reset();
    const TankDimensions tank{500.0f, 1000.0f};
    // Nivel significativo pero presión ~0 => incoherencia => anomalía.
    const VolumeEstimate r = feedManyTimes(f, {450.0f, 0.0f, 15.0f}, tank, 300);

    EXPECT_GT(r.percentage, 5.0f);
    EXPECT_LT(r.pressureBar, 1.0f);
    EXPECT_TRUE(r.anomaly);
}
