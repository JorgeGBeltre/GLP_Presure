#pragma once

#include "Domain/Interfaces/ITiltProvider.h"

namespace glp::infra {

/** Sin IMU: no aporta inclinación (valid=false ⇒ la fusión no corrige). */
class NullTiltProvider : public domain::ITiltProvider {
public:
    void begin() override {}
    domain::TiltReading read() override { return {}; }  // valid=false por default
};

} // namespace glp::infra
