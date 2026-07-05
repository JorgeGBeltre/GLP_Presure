#include <gtest/gtest.h>

#include <cmath>

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

TEST(SensorFusion, InvalidLevelHoldsEstimateAndGrowsUncertainty) {
    SensorFusion f;
    f.reset();
    const TankDimensions tank{500.0f, 1000.0f};

    // Converger con lecturas válidas a 500mm.
    const VolumeEstimate conv = feedManyTimes(f, {500.0f, 6.0f, 15.0f}, tank, 300);
    ASSERT_NEAR(conv.levelMm, 500.0f, 5.0f);
    const float sigmaConverged = conv.kalmanUncertaintyMm;

    // Una lectura de nivel inválida (con un valor basura que NO debe entrar).
    SensorSample bad{0.0f, 6.0f, 15.0f};
    bad.levelValid = false;
    const VolumeEstimate held = f.process(bad, tank);

    EXPECT_NEAR(held.levelMm, 500.0f, 5.0f);              // se sostiene, no cae a 0
    EXPECT_GT(held.kalmanUncertaintyMm, sigmaConverged);  // la incertidumbre crece
}

TEST(SensorFusion, ComputesMassAndVcf) {
    SensorFusion f;
    f.reset();
    const TankDimensions tank{500.0f, 1000.0f};
    const VolumeEstimate r = feedManyTimes(f, {500.0f, 6.0f, 15.0f}, tank, 300);

    EXPECT_GT(r.massKg, 0.0f);
    EXPECT_NEAR(r.massKg, r.densityKgL * r.volumeLiters, 1e-2f); // masa = ρ·V, sin double-count
    EXPECT_GT(r.volume15Liters, 0.0f);
}

TEST(SensorFusion, TiltAxialDebiasRecoversVolume) {
    // Sensor a +1000mm con pitch 8°: la estación lee alto, pero el de-sesgo axial
    // recupera el centro y, a medio tanque, el volumen es ~invariante a la inclinación.
    const TankDimensions tank{500.0f, 1000.0f};
    const float pitch = 8.0f * 3.14159265f / 180.0f;

    SensorFusion flat;
    flat.reset();
    VolumeEstimate rf;
    for (int i = 0; i < 300; ++i) rf = flat.process({500.0f, 6.0f, 15.0f}, tank, 1.0f);

    SensorFusion tilted;
    tilted.reset();
    TiltReading tilt;
    tilt.valid = true;
    tilt.pitchRad = pitch;
    const float station = 500.0f + 1000.0f * std::tan(pitch);
    VolumeEstimate rt;
    for (int i = 0; i < 300; ++i) rt = tilted.process({station, 6.0f, 15.0f}, tank, 1.0f, tilt, 1000.0f);

    EXPECT_NEAR(rt.levelMm, 500.0f, 3.0f);              // de-sesgado al centro
    EXPECT_NEAR(rt.percentage, rf.percentage, 2.0f);   // ~50% ambos
}
