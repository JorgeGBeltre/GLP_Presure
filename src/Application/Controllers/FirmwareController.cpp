#include "Application/Controllers/FirmwareController.h"

namespace glp::app {

void FirmwareController::setup(bool forceProvisioning) {
    config_.load();
    lastConnectTryMs_ = 0;
    connectBackoffMs_ = 0;
    connectAttempts_  = 0;
    state_ = forceProvisioning ? State::Provisioning : State::Connecting;
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

    log_.warn("[PROVISION] sin config tras ventana activa — pasando a espera offline");
    state_ = State::Connecting;
}

void FirmwareController::runConnecting() {
    indicator_.show(domain::Status::Connecting);
    gps_.feed();
    telemetry_.loop();

    const unsigned long now = clock_.nowMs();
    if (!config_.isProvisioned()) {
        if (lastConnectTryMs_ == 0 || (now - lastConnectTryMs_ >= 5000UL)) {
            lastConnectTryMs_ = now;
            config_.load();
        }
        return;
    }

    if (lastConnectTryMs_ != 0 && (now - lastConnectTryMs_ < connectBackoffMs_)) {
        return;
    }
    lastConnectTryMs_ = now;

    if (connectivity_.connect()) {
        connectAttempts_  = 0;
        connectBackoffMs_ = 0;
        telemetry_.resetFilters();
        state_ = State::Operational;
        log_.info("[STATE] -> OPERATIONAL");
    } else {
        // Backoff exponencial no bloqueante (patrón LMU-LTE-1041 CavetMqtt: 2s -> 60s + jitter)
        const uint8_t shift = (connectAttempts_ < 5) ? connectAttempts_ : 5;
        unsigned long baseDelay = (2000UL << shift);
        if (baseDelay > 60000UL) baseDelay = 60000UL;
        connectBackoffMs_ = baseDelay + ((now * 37UL) % 1000UL);
        if (connectAttempts_ < 10) ++connectAttempts_;
        log_.warn("[CONNECT] retry scheduled with exponential backoff");
    }
}

void FirmwareController::runOperational() {
    indicator_.show(domain::Status::Operational);

    if (!connectivity_.isOnline()) {
        if (!connectivity_.ensureConnected()) {
            log_.warn("[OPER] link/MQTT down -> CONNECTING");
            lastConnectTryMs_ = clock_.nowMs();
            connectBackoffMs_ = 2000UL;
            connectAttempts_  = 1;
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
