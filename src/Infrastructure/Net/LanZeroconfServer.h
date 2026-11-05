#pragma once

#include <WebServer.h>
#include <WiFiUdp.h>
#include <cstdint>
#include <functional>
#include <string>
#include "Domain/Interfaces/IConfigRepository.h"
#include "Domain/Interfaces/IIndicator.h"

namespace glp::infra {

/**
 * @brief Servidor HTTP LAN local en el puerto 8081 (/zeroconf/*) + mDNS (_iot._tcp) + UDP :48899
 * Soporta consulta de estado, nivel de señal, actualización Wi-Fi, desvinculación segura
 * y control SLED en red local (texto claro JSON y cifrado AES-128-CBC con MD5(deviceKey)).
 */
class LanZeroconfServer {
public:
    LanZeroconfServer(domain::IIndicator& indicator,
                      domain::IConfigRepository& configRepo,
                      uint16_t port = 8081);

    void setUnlinkCallback(std::function<void()> cb) { unlinkCb_ = std::move(cb); }

    void loop(bool wifiConnected);
    void notifyMdnsStateChange();

private:
    void startServer();
    void stopServer();
    void registerEndpoint(const char* uri);
    void handleUdpDiscovery();
    void dispatchZeroconf(const char* uri, const String& rawBody, char* respBuf, size_t respBufSize);

    domain::IIndicator& indicator_;
    domain::IConfigRepository& configRepo_;
    uint16_t port_;
    std::function<void()> unlinkCb_;

    WebServer server_;
    WiFiUDP udp_;
    bool running_ = false;
    bool udpRunning_ = false;
    bool otaUnlocked_ = false;
    uint32_t sequence_ = 1;
};

} // namespace glp::infra
