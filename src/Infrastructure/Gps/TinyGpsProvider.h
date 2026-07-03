#pragma once

#include "Domain/Interfaces/IGpsProvider.h"
#include "Infrastructure/Gps/gps_module.h"   // driver existente (initGPS, readGPS, getLatitude, ...)

namespace glp::infra {

/** IGpsProvider sobre el driver procedural gps/gps_module (TinyGPS++). */
class TinyGpsProvider : public domain::IGpsProvider {
public:
    void begin() override { initGPS(); }
    void feed() override { readGPS(); }

    domain::GpsFix location() override {
        domain::GpsFix f;
        f.latitude  = getLatitude();
        f.longitude = getLongitude();
        f.valid     = isGPSValid();
        f.timestamp = getGPSTimestamp().c_str();
        return f;
    }
};

} // namespace glp::infra
