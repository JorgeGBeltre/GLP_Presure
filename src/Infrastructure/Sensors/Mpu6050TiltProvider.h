#pragma once

#include <cmath>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "Domain/Interfaces/ITiltProvider.h"

namespace glp::infra {

/**
 * ITiltProvider sobre un MPU-6050 (I2C). Deriva pitch/roll del vector de gravedad
 * del acelerómetro. Si el sensor no responde, begin() deja ok_=false y read()
 * devuelve valid=false (la fusión no corrige).
 *
 * Convención de montaje (importante): el eje **X del IMU** debe alinearse con el
 * eje LONGITUDINAL del tanque, de modo que `pitch` sea la inclinación axial (la que
 * sesga el nivel) y `roll` la rotación de la sección (volumen invariante).
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
