#pragma once

#include "Domain/Entities/RawSensorSample.h"

namespace glp::domain {

/** Acceso al hardware crudo (voltios + tiempos + validez). Sin calibrar. */
class IRawSensorReader {
public:
    virtual ~IRawSensorReader() = default;
    virtual void begin() = 0;
    virtual RawSensorSample read() = 0;
};

} // namespace glp::domain
