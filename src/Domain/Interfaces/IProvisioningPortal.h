#pragma once

namespace glp::domain {

/**
 * Portal de aprovisionamiento (BLE o WiFi AP). Bloquea hasta recibir config
 * o hasta agotar el timeout.
 */
class IProvisioningPortal {
public:
    virtual ~IProvisioningPortal() = default;

    /**
     * Ejecuta el portal.
     * @param timeoutMs tiempo máximo de espera.
     * @return true si se recibió y guardó configuración válida.
     */
    virtual bool run(unsigned long timeoutMs) = 0;
};

} // namespace glp::domain
