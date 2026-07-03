#pragma once

#include <Arduino.h>

#include "Domain/Interfaces/ISystemControl.h"

namespace glp::infra {

/** ISystemControl sobre ESP.restart(). */
class EspSystemControl : public domain::ISystemControl {
public:
    void reboot() override { ESP.restart(); }
};

} // namespace glp::infra
