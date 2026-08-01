#include <gtest/gtest.h>

#include "Domain/Services/StateEkf.h"

using namespace glp::domain;

TEST(StateEkf, ConvergesToConstantMeasurements) {
    StateEkf ekf(0.01f, 0.01f, 4.0f, 0.25f, 1.0f);
    ekf.reset(0.0f, 0.0f);
    for (int i = 0; i < 300; ++i) ekf.update(500.0f, true, 25.0f, true, 25.0f, true);
    EXPECT_NEAR(ekf.level(), 500.0f, 1.0f);
    EXPECT_NEAR(ekf.temp(), 25.0f, 0.2f);
}

TEST(StateEkf, CovariancePositiveAndShrinks) {
    StateEkf ekf(0.01f, 0.01f, 4.0f, 0.25f, 1.0f);
    ekf.reset(0.0f, 0.0f, 1.0e4f);
    const float v0 = ekf.levelVar();
    for (int i = 0; i < 50; ++i) ekf.update(500.0f, true, 25.0f, true, 25.0f, true);
    EXPECT_GT(ekf.levelVar(), 0.0f);
    EXPECT_GT(ekf.tempVar(), 0.0f);
    EXPECT_LT(ekf.levelVar(), v0);
}

TEST(StateEkf, PressureMeasurementReducesTempVariance) {
    // Con la 2ª medida de T (por presión) la varianza de T baja más que sin ella.
    StateEkf withP(0.01f, 0.01f, 4.0f, 0.25f, 0.25f);
    StateEkf noP(0.01f, 0.01f, 4.0f, 0.25f, 0.25f);
    withP.reset(0.0f, 15.0f, 1.0e4f);
    noP.reset(0.0f, 15.0f, 1.0e4f);
    for (int i = 0; i < 20; ++i) {
        withP.update(500.0f, true, 25.0f, true, 25.0f, true);
        noP.update(500.0f, true, 25.0f, true, 0.0f, false);
    }
    EXPECT_LT(withP.tempVar(), noP.tempVar());
}

TEST(StateEkf, InvalidChannelPredictOnlyHoldsAndGrowsVariance) {
    StateEkf ekf(0.05f, 0.05f, 4.0f, 0.25f, 0.25f);
    ekf.reset(500.0f, 25.0f, 1.0f);
    for (int i = 0; i < 50; ++i) ekf.update(500.0f, true, 25.0f, true, 25.0f, true);
    const float lv = ekf.levelVar();
    for (int i = 0; i < 10; ++i) ekf.update(0.0f, false, 25.0f, true, 25.0f, true);
    EXPECT_GT(ekf.levelVar(), lv);              // sólo predijo el nivel ⇒ crece σ
    EXPECT_NEAR(ekf.level(), 500.0f, 5.0f);     // se sostiene, no cae a 0
}
