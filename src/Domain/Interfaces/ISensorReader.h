#pragma once

#include "Domain/Entities/SensorSample.h"

namespace glp::domain {

/** Acceso a los sensores físicos (ultrasónico, presión, temperatura). */
class ISensorReader {
public:
    virtual ~ISensorReader() = default;

    /** Inicializa pines/ADC. */
    virtual void begin() = 0;

    /** Devuelve una lectura cruda de los tres sensores. */
    virtual SensorSample read() = 0;
};

} // namespace glp::domain
