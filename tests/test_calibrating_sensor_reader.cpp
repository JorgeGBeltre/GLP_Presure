#include <gtest/gtest.h>

#include "Domain/Services/CalibratingSensorReader.h"
#include "Domain/Services/SensorCalibrator.h"
#include "support/Fakes.h"

using namespace glp::domain;
using namespace glp::testing;

TEST(CalibratingSensorReader, ConvertsRawToPhysicalAndPropagatesValidity) {
    FakeRawSensorReader raw;
    DeviceConfig cfg;
    cfg.tankRadiusCm    = 50.0f;   // 2R = 1000mm
    cfg.tankHeightCm    = 100.0f;  // H = 1000mm
    cfg.mountOffsetCm   = 0.0f;
    cfg.refDistanceCm   = 10.0f;   // reflector a 100mm ⇒ c determinista
    cfg.propaneFraction = 1.0f;
    cfg.calibration.pressSlope  = 20.0f;  cfg.calibration.pressOffset = -10.0f;
    cfg.calibration.tempRRef    = 1000.0f; cfg.calibration.tempVExc   = 3.30f;
    cfg.calibration.tempOffsetC = 0.0f;

    // Reflector: tRef=800µs a 100mm ⇒ c = 2000·100/800 = 250 m/s.
    raw.next.ultrasonic.refValid  = true;
    raw.next.ultrasonic.tRefUs    = 800.0f;
    // Eco del líquido: tFlight=4000µs ⇒ d = 250·4000/2000 = 500mm ⇒ nivel = 1000−500 = 500.
    raw.next.ultrasonic.echoValid = true;
    raw.next.ultrasonic.tFlightUs = 4000.0f;
    raw.next.pressureVolts  = 1.5f;   raw.next.pressureValid = true;
    const float rpt = 1097.347f;
    raw.next.tempVolts = 3.30f * rpt / (rpt + 1000.0f); raw.next.tempValid = true;

    CalibratingSensorReader reader{raw, cfg};
    const SensorSample s = reader.read();

    EXPECT_NEAR(s.levelRawMm, 500.0f, 1.0f);   // reflector c=250, tFlight=4000 ⇒ d=500, h=500
    EXPECT_NEAR(s.pressureRawBar, 20.0f, 1e-3f);
    EXPECT_NEAR(s.tempRawC, 25.0f, 0.05f);
    EXPECT_TRUE(s.levelValid);
    EXPECT_TRUE(s.reflectorUsed);
    EXPECT_TRUE(s.pressureValid);
    EXPECT_TRUE(s.tempValid);
}

TEST(CalibratingSensorReader, PropagatesInvalidFlags) {
    FakeRawSensorReader raw;
    DeviceConfig cfg;
    raw.next.ultrasonic.echoValid = false; raw.next.pressureValid = false; raw.next.tempValid = false;

    CalibratingSensorReader reader{raw, cfg};
    const SensorSample s = reader.read();

    EXPECT_FALSE(s.levelValid);
    EXPECT_FALSE(s.pressureValid);
    EXPECT_FALSE(s.tempValid);
}
