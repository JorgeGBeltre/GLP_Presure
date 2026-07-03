#pragma once

#include <Arduino.h>

#include "Domain/Interfaces/IClock.h"

namespace glp::infra {

/** IClock sobre millis()/delay() del núcleo Arduino. */
class ArduinoClock : public domain::IClock {
public:
    unsigned long nowMs() override { return millis(); }
    void sleepMs(unsigned long ms) override { delay(ms); }
};

} // namespace glp::infra
