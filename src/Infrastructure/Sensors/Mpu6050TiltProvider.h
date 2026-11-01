#pragma once

#include <cmath>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "Domain/Interfaces/ITiltProvider.h"

namespace glp::infra {

/**
 * ITiltProvider sobre un MPU-6050 (I2C). Deriva pitch/roll del vector de gravedad
 * del acelerómetro y detecta choques mecánicos o actividad sísmica (GTCRA).
 */
class Mpu6050TiltProvider : public domain::ITiltProvider {
public:
    void begin() override { ok_ = mpu_.begin(); }

    domain::TiltReading read() override {
        domain::TiltReading t;
        if (!ok_) return t;                       // valid=false

        sensors_event_t a, g, temp;
        mpu_.getEvent(&a, &g, &temp);
        const float ax = a.acceleration.x;
        const float ay = a.acceleration.y;
        const float az = a.acceleration.z;

        const float gMag = std::sqrt(ax * ax + ay * ay + az * az);
        t.accelMagnitudeG = gMag / 9.80665f;
        t.seismicImpactDetected = (std::fabs(t.accelMagnitudeG - 1.0f) >= 1.2f);

        if (gMag < 3.0f || gMag > 25.0f) {
            return t; // Aceleración anómala o impacto fuerte: no alterar calibración estática de inclinación
        }

        t.pitchRad = std::atan2(-ax, std::sqrt(ay * ay + az * az));
        t.rollRad  = std::atan2(ay, az);
        t.valid    = true;
        return t;
    }

private:
    Adafruit_MPU6050 mpu_;
    bool             ok_ = false;
};

} // namespace glp::infra
