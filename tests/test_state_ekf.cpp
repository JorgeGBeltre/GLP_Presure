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

TEST(StateEkf, NonlinearPressureConvergesWithAnalyticalJacobian) {
    StateEkf ekf(0.005f, 0.01f, 2.5f, 0.25f, 0.5f);
    ekf.reset(0.0f, 15.0f);

    // Supongamos nivel 600 mm, T real 25 °C, P real ~9.77 bar (propano)
    const float trueH = 600.0f;
    const float trueT = 25.0f;
    const float pSatTrue = 9.77f;

    for (int i = 0; i < 200; ++i) {
        const float tPrior = ekf.temp();
        // Modelo simplificado para la prueba: pSat y dP_dT aproximados alrededor de 25°C
        const float pSat = pSatTrue + 0.25f * (tPrior - trueT);
        const float dP_dT = 0.25f; // bar/°C
        ekf.updateNonlinear(trueH, true, trueT, true, pSatTrue, true, pSat, dP_dT);
    }

    EXPECT_NEAR(ekf.level(), trueH, 1.0f);
    EXPECT_NEAR(ekf.temp(), trueT, 0.2f);
    EXPECT_LT(ekf.levelVar(), 1.0f);
    EXPECT_LT(ekf.tempVar(), 0.1f);
}

TEST(StateEkf, MahalanobisGateRejectsPressureOutliers) {
    StateEkf ekf(0.005f, 0.01f, 2.5f, 0.25f, 0.5f);
    ekf.reset(500.0f, 20.0f, 0.1f); // ya convergido a 20°C

    // Lectura de presión anómala / ruido salvaje: 50.0 bar (muy por encima de 3-sigma)
    const float pSatExpected = 8.59f; // a 20°C
    const float dP_dT = 0.22f;
    const float tBefore = ekf.temp();

    ekf.updateNonlinear(500.0f, true, 20.0f, true, 50.0f, true, pSatExpected, dP_dT, true);

    // Debe haber rechazado la presión anómala y conservado la temperatura de 20°C
    EXPECT_NEAR(ekf.temp(), tBefore, 0.5f);
}
