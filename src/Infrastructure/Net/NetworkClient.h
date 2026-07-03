#pragma once

#include <WiFi.h>

namespace glp::infra {

/**
 * Cliente TCP para el transporte MQTT según el enlace activo:
 *   - WiFi STA conectado  -> WiFiClient (TCP directo por WiFi)
 *   - si no               -> TinyGsmClientBridge (TCP por el módem celular)
 * Ambos son WiFiClient& (el bridge hereda de WiFiClient), que es lo que
 * ESP32_MQTTv5::begin(WiFiClient&, ...) espera.
 */
WiFiClient& getActiveNetworkClient();

} // namespace glp::infra
