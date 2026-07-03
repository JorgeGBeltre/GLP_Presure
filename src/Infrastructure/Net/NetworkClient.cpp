#include "Infrastructure/Net/NetworkClient.h"

#include "Infrastructure/Net/sim_module.h"   // TinyGsmClientBridge& getSimClient()

namespace glp::infra {

WiFiClient& getActiveNetworkClient() {
    static WiFiClient wifiClient;
    if (WiFi.status() == WL_CONNECTED) {
        return wifiClient;                 // TCP directo por WiFi
    }
    return ::getSimClient();               // TinyGsmClientBridge : WiFiClient
}

} // namespace glp::infra
