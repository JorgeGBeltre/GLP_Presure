#pragma once

#include <Adafruit_ADS1X15.h>

#include "Domain/Interfaces/IRawSensorReader.h"
#include "Infrastructure/Sensors/sensor_module.h"

namespace glp::infra {

/** IRawSensorReader: ultrasónico (mm+validez) + ADS1115 (presión y temp en voltios). */
class Esp32RawSensorReader : public domain::IRawSensorReader {
public:
    void begin() override {
        initSensors();          // pines del ultrasónico
        ads_.begin();           // I2C 0x48 por defecto
        ads_.setGain(GAIN_ONE); // ±4.096V FSR
    }

    domain::RawSensorSample read() override {
        domain::RawSensorSample r;
        r.echoValid = readUltrasonicRaw(r.echoDistanceMm);

        r.pressureVolts = ads_.computeVolts(ads_.readADC_SingleEnded(kPressureCh));
        r.pressureValid = (r.pressureVolts > 0.02f); // >20mV = no está en el riel bajo

        r.tempVolts = ads_.computeVolts(ads_.readADC_SingleEnded(kTempCh));
        r.tempValid = (r.tempVolts > 0.02f && r.tempVolts < 3.28f); // ni cortocircuito ni abierto

        return r;
    }

private:
    static constexpr uint8_t kPressureCh = 0;
    static constexpr uint8_t kTempCh     = 1;
    Adafruit_ADS1115 ads_;
};

} // namespace glp::infra
