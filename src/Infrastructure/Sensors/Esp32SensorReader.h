#pragma once

#include "Domain/Interfaces/ISensorReader.h"
#include "Infrastructure/Sensors/sensor_module.h"   // driver existente (initSensors, read* )

namespace glp::infra {

/** ISensorReader sobre el driver procedural sensors/sensor_module. */
class Esp32SensorReader : public domain::ISensorReader {
public:
    void begin() override { initSensors(); }

    domain::SensorSample read() override {
        domain::SensorSample s;
        s.levelRawMm     = readUltrasonicRawMM();
        s.pressureRawBar = readPressureRawBar();
        s.tempRawC       = readTemperatureRawC();
        return s;
    }
};

} // namespace glp::infra
