#pragma once

#include "Domain/Interfaces/IButtonHandler.h"
#include "Domain/Interfaces/IIndicator.h"
#include "Domain/Interfaces/IProvisioningPortal.h"
#include "Infrastructure/Provisioning/ble_provisioning.h"
#include "Infrastructure/Provisioning/wifi_ap_provisioning.h"

namespace glp::infra {

/** Portal Quick Pairing (BLE GATT + SmartConfig ESP-Touch/AirKiss). */
class BleProvisioningPortal : public domain::IProvisioningPortal {
public:
    bool run(unsigned long timeoutMs) override { return runBLEProvisioning(timeoutMs); }

    QuickPairingResult runInteractive(unsigned long timeoutMs,
                                      domain::IIndicator& indicator,
                                      domain::IButtonHandler& button) {
        return runQuickPairing(timeoutMs, &indicator, &button);
    }
};

/** Portal SoftAP + formulario HTTP (`GLP-CONFIG-<DeviceID>`). */
class WifiApProvisioningPortal : public domain::IProvisioningPortal {
public:
    bool run(unsigned long timeoutMs) override { return runWiFiAPProvisioning(timeoutMs); }

    bool runInteractive(unsigned long timeoutMs,
                        domain::IIndicator& indicator,
                        domain::IButtonHandler& button) {
        return runWiFiAPProvisioning(timeoutMs, &indicator, &button);
    }
};

} // namespace glp::infra
