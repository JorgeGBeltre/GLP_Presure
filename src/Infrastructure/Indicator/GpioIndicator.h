#pragma once

#include <Arduino.h>
#include "Domain/Interfaces/IIndicator.h"

namespace glp::infra {

/**
 * @brief Generador de patrones no bloqueante para el LED de estado (SLED) con base de 100 ms:
 * - Provisioning / QuickPairing: 100 ms ON, 100 ms OFF, 100 ms ON, 700 ms OFF (2 destellos rápidos + pausa)
 * - CompatibleAp / Anomaly:      100 ms ON / 100 ms OFF continuo (5 Hz)
 * - Connecting:                  200 ms ON / 1800 ms OFF (1 destello corto cada 2 s)
 * - OtaProgress / Error:         3 destellos rápidos + pausa (100 ms ON/OFF x3 + 400 ms OFF)
 * - Operational:                 Encendido fijo si sledOnline=true, Apagado si sledOnline=false
 */
class GpioIndicator : public domain::IIndicator {
public:
    static constexpr unsigned long BASE_TICK_MS = 100;

    explicit GpioIndicator(uint8_t pin) : pin_(pin) {
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
    }

    void setSledOnline(bool enabled) override {
        sledOnline_ = enabled;
        if (status_ == domain::IndicatorStatus::Operational) {
            writePin(sledOnline_);
        }
    }

    bool isSledOnline() const override {
        return sledOnline_;
    }

    void show(domain::IndicatorStatus status) override {
        if (status_ != status) {
            status_ = status;
            tickStep_ = 0;
            lastTickMs_ = 0;
        }
        if (status_ == domain::IndicatorStatus::Operational) {
            writePin(sledOnline_);
        } else {
            update();
        }
    }

    void update() override {
        if (status_ == domain::IndicatorStatus::Operational) {
            writePin(sledOnline_);
            return;
        }

        const unsigned long now = millis();
        if (lastTickMs_ != 0 && (now - lastTickMs_) < BASE_TICK_MS) {
            return;
        }
        lastTickMs_ = now;

        bool turnOn = false;
        switch (status_) {
            case domain::IndicatorStatus::Provisioning:
            case domain::IndicatorStatus::QuickPairing: {
                const uint32_t phase = tickStep_ % 10U;
                turnOn = (phase == 0U || phase == 2U);
                break;
            }

            case domain::IndicatorStatus::CompatibleAp:
            case domain::IndicatorStatus::Anomaly:
                turnOn = (tickStep_ % 2U) == 0U;
                break;

            case domain::IndicatorStatus::Connecting:
                turnOn = (tickStep_ % 20U) < 2U;
                break;

            case domain::IndicatorStatus::OtaProgress:
            case domain::IndicatorStatus::Error: {
                const uint32_t phase = tickStep_ % 10U;
                turnOn = (phase == 0U || phase == 2U || phase == 4U);
                break;
            }

            case domain::IndicatorStatus::Operational:
            default:
                turnOn = sledOnline_;
                break;
        }

        ++tickStep_;
        writePin(turnOn);
    }

private:
    void writePin(bool on) {
        digitalWrite(pin_, on ? HIGH : LOW);
    }

    uint8_t pin_;
    domain::IndicatorStatus status_ = domain::IndicatorStatus::Connecting;
    unsigned long lastTickMs_ = 0;
    uint32_t tickStep_ = 0;
    bool sledOnline_ = true;
};

} // namespace glp::infra
