#pragma once

namespace glp::domain {

/** Control del sistema (reinicio). En el firmware envuelve ESP.restart(). */
class ISystemControl {
public:
    virtual ~ISystemControl() = default;
    virtual void reboot() = 0;
};

} // namespace glp::domain
