#pragma once

namespace glp::domain {

enum class IndicatorStatus {
    Provisioning,   // Esperando configuración inicial (compatible con QuickPairing)
    QuickPairing,   // Modo Quick Pairing BLE + SmartConfig (2 destellos rápidos + pausa: 100/100/100/700 ms)
    CompatibleAp,   // Modo SoftAP GLP-CONFIG-<DeviceID> (Parpadeo continuo rápido: 100 ms ON / 100 ms OFF)
    Connecting,     // Conectando a red / MQTTv5 (200 ms ON / 1800 ms OFF)
    Operational,    // Ciclo normal (Encendido fijo si sledOnline=true, Apagado si sledOnline=false)
    OtaProgress,    // Descarga OTA en curso (3 destellos rápidos + pausa)
    Anomaly,        // Lectura anómala detectada (parpadeo rápido)
    Error           // Fallo crítico
};

using Status = IndicatorStatus;

/** Salida visual de estado (LED de placa, RGB, display, etc.). */
class IIndicator {
public:
    virtual ~IIndicator() = default;
    virtual void show(IndicatorStatus status) = 0;
    virtual void update() {}
    virtual void setSledOnline(bool enabled) { (void)enabled; }
    virtual bool isSledOnline() const { return true; }
};

} // namespace glp::domain
