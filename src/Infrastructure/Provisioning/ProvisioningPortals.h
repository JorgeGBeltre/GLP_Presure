#pragma once

#include "Domain/Interfaces/IProvisioningPortal.h"
#include "Infrastructure/Provisioning/ble_provisioning.h"
#include "Infrastructure/Provisioning/smartconfig_provisioning.h"
#include "Infrastructure/Provisioning/wifi_ap_provisioning.h"

namespace glp::infra {

/** Portal BLE (NimBLE): config de negocio + credenciales WiFi. */
class BleProvisioningPortal : public domain::IProvisioningPortal {
public:
    bool run(unsigned long timeoutMs) override { return runBLEProvisioning(timeoutMs); }
};

/** Portal SoftAP / portal cautivo: config de negocio + credenciales WiFi. */
class WifiApProvisioningPortal : public domain::IProvisioningPortal {
public:
    bool run(unsigned long timeoutMs) override { return runWiFiAPProvisioning(timeoutMs); }
};

/** Portal SmartConfig / EspTouch (nativo): sólo credenciales WiFi. */
class SmartConfigPortal : public domain::IProvisioningPortal {
public:
    bool run(unsigned long timeoutMs) override { return runSmartConfigProvisioning(timeoutMs); }
};

} // namespace glp::infra
