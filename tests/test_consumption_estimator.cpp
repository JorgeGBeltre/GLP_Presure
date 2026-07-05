#include <gtest/gtest.h>

#include "Domain/Services/ConsumptionEstimator.h"

using namespace glp::domain;

static constexpr unsigned long kDayMs = 86400000UL;

TEST(ConsumptionEstimator, ComputesDailyRateFromVolumeDrop) {
    ConsumptionEstimator c;
    c.reset();
    c.update(1000.0f, 0);          // sembrar
    c.update(990.0f, kDayMs);      // −10 L en 1 día => 10 L/día
    EXPECT_NEAR(c.litersPerDay(), 10.0f, 0.5f);
}

TEST(ConsumptionEstimator, RemainingDaysIsVolumeOverRate) {
    ConsumptionEstimator c;
    c.reset();
    c.update(1000.0f, 0);
    c.update(990.0f, kDayMs);      // 10 L/día
    EXPECT_NEAR(c.remainingDays(990.0f), 99.0f, 5.0f);
}

TEST(ConsumptionEstimator, RefillDoesNotProduceNegativeConsumption) {
    ConsumptionEstimator c;
    c.reset();
    c.update(500.0f, 0);
    c.update(1000.0f, kDayMs);     // recarga (+500 L): no es consumo
    EXPECT_GE(c.litersPerDay(), 0.0f);
}

TEST(ConsumptionEstimator, NoRateBeforeSecondSample) {
    ConsumptionEstimator c;
    c.reset();
    c.update(1000.0f, 0);
    EXPECT_FLOAT_EQ(c.litersPerDay(), 0.0f);
    EXPECT_FLOAT_EQ(c.remainingDays(1000.0f), 0.0f);
}
