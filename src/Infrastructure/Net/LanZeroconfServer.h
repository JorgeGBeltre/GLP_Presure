#pragma once

#include <WebServer.h>
#include <cstdint>
#include <string>
#include "Domain/Interfaces/IConfigRepository.h"
#include "Domain/Interfaces/IIndicator.h"

namespace glp::infra {

/**
 * @brief Servidor HTTP LAN local en el puerto 8081 (/zeroconf/*) + mDNS
 * Soporta consulta de estado, nivel de señal, actualización Wi-Fi y control SLED en red local
 * (texto claro JSON y cifrado AES-128-CBC con MD5(deviceKey)).
 */
class LanZeroconfServer {
public:
    LanZeroconfServer(domain::IIndicator& indicator,
                      domain::IConfigRepository& configRepo,
                      uint16_t port = 8081);

    void loop(bool wifiConnected);
    void notifyMdnsStateChange();

private:
    void startServer();
    void stopServer();
    void registerEndpoint(const char* uri);
    void dispatchZeroconf(const char* uri, const String& rawBody, char* respBuf, size_t respBufSize);

    domain::IIndicator& indicator_;
    domain::IConfigRepository& configRepo_;
    uint16_t port_;

    WebServer server_;
    bool running_ = false;
    bool otaUnlocked_ = false;
    uint32_t sequence_ = 1;
};

} // namespace glp::infra

