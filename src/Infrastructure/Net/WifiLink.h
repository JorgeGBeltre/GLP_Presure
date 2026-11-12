#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "Domain/Entities/DeviceConfig.h"
#include "Domain/Interfaces/INetworkLink.h"
#include "Infrastructure/System/EspSystemControl.h"

namespace glp::infra {

/**
 * INetworkLink WiFi (modo estación). Lee SSID/clave de la config viva; se
 * considera "configurado" sólo si hay SSID. Uplink preferido cuando existe.
 */
class WifiLink : public domain::INetworkLink {
public:
    explicit WifiLink(const domain::DeviceConfig& cfg) : cfg_(cfg) {}

    domain::LinkType type() const override { return domain::LinkType::Wifi; }
    bool isConfigured() override { return !cfg_.wifiSsid.empty(); }

    bool connect() override {
        if (!isConfigured()) return false;
        if (WiFi.status() == WL_CONNECTED) return true;
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfg_.wifiSsid.c_str(), cfg_.wifiPass.c_str());
        const unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 3500UL) {
            EspSystemControl::heartbeat();
            delay(100);
        }
        return WiFi.status() == WL_CONNECTED;
    }

    bool isConnected() override { return WiFi.status() == WL_CONNECTED; }
    int  rssi() override { return WiFi.RSSI(); }
    std::string ipAddress() override { return WiFi.localIP().toString().c_str(); }

private:
    const domain::DeviceConfig& cfg_;
};

} // namespace glp::infra
