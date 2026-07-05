#pragma once

#include "Domain/Entities/TiltReading.h"

namespace glp::domain {

/** Proveedor de inclinación (IMU). La implementación nula devuelve valid=false. */
class ITiltProvider {
public:
    virtual ~ITiltProvider() = default;
    virtual void begin() = 0;
    virtual TiltReading read() = 0;
};

} // namespace glp::domain
