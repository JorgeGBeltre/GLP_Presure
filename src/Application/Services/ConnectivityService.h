#pragma once

#include "Domain/Interfaces/ILogger.h"
#include "Domain/Interfaces/INetworkLink.h"
#include "Domain/Interfaces/ITelemetryTransport.h"

namespace glp::app {

/**
 * Orquesta conectividad DUAL: WiFi (preferido si está configurado) con fallback
 * a celular (SIM). Sobre el enlace activo levanta el transporte MQTT. No conoce
 * WiFi.h, TinyGSM ni ESP32_MQTTv5, sólo las interfaces de dominio.
 */
class ConnectivityService {
public:
    ConnectivityService(domain::INetworkLink& wifi,
                        domain::INetworkLink& cellular,
                        domain::ITelemetryTransport& transport,
                        domain::ILogger& log)
        : wifi_(wifi), cellular_(cellular), transport_(transport), log_(log) {}

    /** Trae un enlace (WiFi si hay SSID, si no celular) y luego MQTT. */
    bool connect();

    /** true si el enlace activo y MQTT están conectados. */
    bool isOnline();

    /**
     * Desde OPERATIONAL: si el enlace activo cayó devuelve false (rehacer
     * CONNECTING); si sólo cayó MQTT intenta reconectar.
     */
    bool ensureConnected();

    domain::ITelemetryTransport& transport() { return transport_; }

    /** RSSI del enlace activo (0 si no hay). */
    int activeRssi();

    /** Tipo del enlace activo. */
    domain::LinkType activeType() const { return activeType_; }

    /** Nombre del tipo de enlace activo ("Wifi" | "SimCard"). */
    std::string activeTypeString() const { return domain::toString(activeType_); }

    /** IP local del enlace activo ("0.0.0.0" si no hay). */
    std::string activeIp();

private:
    domain::INetworkLink& activeLink();
    bool hasActiveLink();

    domain::INetworkLink&        wifi_;
    domain::INetworkLink&        cellular_;
    domain::ITelemetryTransport& transport_;
    domain::ILogger&             log_;

    domain::LinkType activeType_ = domain::LinkType::Cellular;
    bool             haveLink_   = false;
};

} // namespace glp::app
