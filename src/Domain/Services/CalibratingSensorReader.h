#pragma once

#include "Domain/Entities/DeviceConfig.h"
#include "Domain/Entities/SensorSample.h"
#include "Domain/Interfaces/IRawSensorReader.h"
#include "Domain/Interfaces/ISensorReader.h"
#include "Domain/Services/SensorCalibrator.h"
#include "Domain/Services/TankGeometry.h"

namespace glp::domain {

/**
 * ISensorReader que envuelve un IRawSensorReader y aplica calibración leyendo la
 * config VIVA (referencia a DeviceConfig): así una actualización remota surte
 * efecto sin reconstruir nada. PURO, testeable en host.
 */
class CalibratingSensorReader : public ISensorReader {
public:
    CalibratingSensorReader(IRawSensorReader& raw, const DeviceConfig& cfg)
        : raw_(raw), cfg_(cfg) {}

    void begin() override { raw_.begin(); }

    SensorSample read() override {
        const RawSensorSample r = raw_.read();
        const SensorCalibration& c = cfg_.calibration;

        SensorSample s;
        s.levelValid    = r.echoValid;
        s.pressureValid = r.pressureValid;
        s.tempValid     = r.tempValid;

        const float radiusMm = cfg_.tankRadiusCm  * 10.0f;
        const float heightMm = cfg_.tankHeightCm  * 10.0f;
        const float mountMm  = cfg_.mountOffsetCm * 10.0f;
        s.levelRawMm = TankGeometry::levelFromDistance(r.echoDistanceMm, heightMm, mountMm, radiusMm);

        s.pressureRawBar = SensorCalibrator::voltsToBar(r.pressureVolts, c.pressSlope, c.pressOffset);

        const float rpt = SensorCalibrator::resistanceFromVolts(r.tempVolts, c.tempRRef, c.tempVExc);
        s.tempRawC = SensorCalibrator::pt1000ToCelsius(rpt) + c.tempOffsetC;

        return s;
    }

private:
    IRawSensorReader&   raw_;
    const DeviceConfig& cfg_;
};

} // namespace glp::domain
