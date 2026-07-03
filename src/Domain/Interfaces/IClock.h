#pragma once

namespace glp::domain {

/** Reloj monótono. En el firmware envuelve millis(); en tests es controlable. */
class IClock {
public:
    virtual ~IClock() = default;

    /** Milisegundos desde el arranque. */
    virtual unsigned long nowMs() = 0;

    /** Espera bloqueante (delay). En tests puede ser no-op. */
    virtual void sleepMs(unsigned long ms) = 0;
};

} // namespace glp::domain
