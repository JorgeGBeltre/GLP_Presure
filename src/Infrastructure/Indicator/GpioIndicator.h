#pragma once

#include <Arduino.h>

#include "Domain/Interfaces/IIndicator.h"

namespace glp::infra {

/**
 * IIndicator sobre un LED GPIO. Mapea cada estado a un patrón de parpadeo,
 * replicando el comportamiento del antiguo main.cpp (ledBlink).
 */
class GpioIndicator : public domain::IIndicator {
public:
    explicit GpioIndicator(int pin) : pin_(pin) { pinMode(pin_, OUTPUT); }

    void show(domain::Status s) override {
        using domain::Status;
        switch (s) {
            case Status::Provisioning: blink(100, 100, 3); break;
            case Status::Connecting:   blink(500, 500, 1); break;
            case Status::Operational:  digitalWrite(pin_, HIGH); break;
            case Status::Anomaly:      blink(50, 50, 6); break;
            case Status::Error:        blink(1000, 200, 2); break;
        }
    }

private:
    void blink(int onMs, int offMs, int cycles) {
        for (int i = 0; i < cycles; ++i) {
            digitalWrite(pin_, HIGH); delay(onMs);
            digitalWrite(pin_, LOW);  delay(offMs);
        }
    }
    int pin_;
};

} // namespace glp::infra
