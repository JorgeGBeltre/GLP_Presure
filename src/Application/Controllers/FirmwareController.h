#pragma once

#include <vector>

#include "Application/Services/ConfigService.h"
#include "Application/Services/ConnectivityService.h"
#include "Application/Services/TelemetryService.h"
#include "Domain/Interfaces/IClock.h"
#include "Domain/Interfaces/IGpsProvider.h"
#include "Domain/Interfaces/IIndicator.h"
#include "Domain/Interfaces/ILogger.h"
#include "Domain/Interfaces/IOtaService.h"
#include "Domain/Interfaces/IProvisioningPortal.h"
#include "Domain/Interfaces/ISystemControl.h"

namespace glp::app {

/**
 * Máquina de estados del firmware:
 *
 *   PROVISIONING → CONNECTING → OPERATIONAL  (reingreso a CONNECTING si cae la red)
 *
 * El provisioning prueba una lista ordenada de transportes (BLE, SoftAP,
 * SmartConfig...) hasta que uno entregue configuración.
 */
class FirmwareController {
public:
    enum class State { Provisioning, Connecting, Operational };

    /** Un transporte de provisioning con su timeout. */
    struct ProvisioningStep {
        domain::IProvisioningPortal* portal;
        unsigned long                timeoutMs;
    };

    FirmwareController(ConfigService& config,
                       std::vector<ProvisioningStep> provisioning,
                       ConnectivityService& connectivity,
                       TelemetryService& telemetry,
                       domain::IGpsProvider& gps,
                       domain::IOtaService& ota,
                       domain::IIndicator& indicator,
                       domain::ISystemControl& system,
                       domain::IClock& clock,
                       domain::ILogger& log)
        : config_(config), provisioning_(std::move(provisioning)),
          connectivity_(connectivity), telemetry_(telemetry), gps_(gps),
          ota_(ota), indicator_(indicator), system_(system), clock_(clock),
          log_(log) {}

    void setup(bool forceProvisioning = false);
    void tick();
    State state() const { return state_; }

private:
    void runProvisioning();
    void runConnecting();
    void runOperational();

    ConfigService&                config_;
    std::vector<ProvisioningStep> provisioning_;
    ConnectivityService&          connectivity_;
    TelemetryService&             telemetry_;
    domain::IGpsProvider&         gps_;
    domain::IOtaService&          ota_;
    domain::IIndicator&           indicator_;
    domain::ISystemControl&       system_;
    domain::IClock&               clock_;
    domain::ILogger&              log_;

    State state_ = State::Provisioning;
};

} // namespace glp::app
