#include "Application/Services/ConnectivityService.h"

namespace glp::app {

using domain::LinkType;

bool ConnectivityService::connect() {
    haveLink_ = false;

    if (wifi_.isConfigured() && wifi_.connect()) {
        activeType_ = LinkType::Wifi;
        haveLink_ = true;
        log_.info("[CONN] enlace WiFi arriba");
    } else if (cellular_.connect()) {
        activeType_ = LinkType::Cellular;
        haveLink_ = true;
        log_.info("[CONN] enlace celular arriba");
    } else {
        log_.warn("[CONN] ningun enlace disponible");
        return false;
    }

    if (!transport_.connect()) {
        log_.warn("[CONN] MQTT fallo");
        return false;
    }
    log_.info("[CONN] online");
    return true;
}

domain::INetworkLink& ConnectivityService::activeLink() {
    return (activeType_ == LinkType::Wifi) ? wifi_ : cellular_;
}

bool ConnectivityService::hasActiveLink() {
    return haveLink_ && activeLink().isConnected();
}

bool ConnectivityService::isOnline() {
    return hasActiveLink() && transport_.isConnected();
}

int ConnectivityService::activeRssi() {
    return haveLink_ ? activeLink().rssi() : 0;
}

std::string ConnectivityService::activeIp() {
    return haveLink_ ? activeLink().ipAddress() : "0.0.0.0";
}

bool ConnectivityService::ensureConnected() {
    if (!hasActiveLink()) {
        // El enlace activo cayó: rehacer todo el flujo CONNECTING.
        return false;
    }
    if (!transport_.isConnected()) {
        log_.info("[CONN] MQTT reconectando");
        return transport_.connect();
    }
    return true;
}

} // namespace glp::app
