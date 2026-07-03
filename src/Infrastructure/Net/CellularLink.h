#pragma once

#include "Domain/Interfaces/INetworkLink.h"
#include "Infrastructure/Net/sim_module.h"   // initSIM, isSimConnected, getSignalRSSI

namespace glp::infra {

/** INetworkLink celular (SIM7600 vía TinyGSM). Fallback siempre disponible. */
class CellularLink : public domain::INetworkLink {
public:
    domain::LinkType type() const override { return domain::LinkType::Cellular; }
    bool isConfigured() override { return true; } // APN en NVS; el celular es el fallback
    bool connect() override { return initSIM(); }
    bool isConnected() override { return isSimConnected(); }
    int  rssi() override { return getSignalRSSI(); }
    std::string ipAddress() override { return getSimLocalIp(); }
};

} // namespace glp::infra
