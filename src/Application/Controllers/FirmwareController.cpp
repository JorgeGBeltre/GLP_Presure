#include "Application/Controllers/FirmwareController.h"

namespace glp::app {

void FirmwareController::setup(bool forceProvisioning) {
    config_.load();
    state_ = (config_.isProvisioned() && !forceProvisioning)
                 ? State::Connecting
                 : State::Provisioning;
}

void FirmwareController::tick() {
    switch (state_) {
        case State::Provisioning: runProvisioning(); break;
        case State::Connecting:   runConnecting();   break;
        case State::Operational:  runOperational();  break;
    }
}

void FirmwareController::runProvisioning() {
    indicator_.show(domain::Status::Provisioning);

    // Prueba cada transporte en orden hasta que uno entregue configuración.
    for (auto& step : provisioning_) {
        if (step.portal && step.portal->run(step.timeoutMs)) {
            log_.info("[PROVISION] config guardada — reboot");
            system_.reboot();
            return;
        }
    }

    log_.warn("[PROVISION] ningun transporte entrego config — reboot");
    system_.reboot();
}

void FirmwareController::runConnecting() {
    indicator_.show(domain::Status::Connecting);

    if (connectivity_.connect()) {
        telemetry_.resetFilters();
        state_ = State::Operational;
        log_.info("[STATE] -> OPERATIONAL");
    } else {
        log_.warn("[CONNECT] retry");
    }
}

void FirmwareController::runOperational() {
    indicator_.show(domain::Status::Operational);

    if (!connectivity_.isOnline()) {
        if (!connectivity_.ensureConnected()) {
            log_.warn("[OPER] modem down -> CONNECTING");
            state_ = State::Connecting;
            return;
        }
    }

    connectivity_.transport().loop();
    ota_.handle();
    gps_.feed();
    telemetry_.loop();
}

} // namespace glp::app
