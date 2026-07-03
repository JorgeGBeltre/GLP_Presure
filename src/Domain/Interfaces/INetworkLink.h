#pragma once

#include <string>

namespace glp::domain {

enum class LinkType { Wifi, Cellular };

/** Nombre del tipo de enlace para la cabecera "Device.TypeConexion". */
inline const char* toString(LinkType t) {
    return (t == LinkType::Wifi) ? "Wifi" : "SimCard";
}

/**
 * Un enlace de red (uplink). El firmware tiene dos implementaciones: WiFi y
 * celular (SIM). La Application decide cuál usar sin conocer WiFi.h ni TinyGSM.
 *
 * Nota: intencionalmente NO expone el `Client` de Arduino — de qué objeto se
 * sirve el transporte MQTT es un detalle de Infrastructure.
 */
class INetworkLink {
public:
    virtual ~INetworkLink() = default;

    virtual LinkType type() const = 0;

    /** true si este enlace está configurado (p.ej. WiFi con SSID no vacío). */
    virtual bool isConfigured() = 0;

    /** Levanta el enlace. Devuelve true si quedó conectado. */
    virtual bool connect() = 0;

    virtual bool isConnected() = 0;

    /** Intensidad de señal (dBm para WiFi/celular); 0 si no aplica. */
    virtual int rssi() = 0;

    /** Dirección IP local del enlace (o "0.0.0.0" si no aplica). */
    virtual std::string ipAddress() = 0;
};

} // namespace glp::domain
