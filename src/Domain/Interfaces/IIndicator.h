#pragma once

namespace glp::domain {

/** Indicador de estado (LED). El firmware mapea cada estado a un patrón. */
enum class Status {
    Provisioning, // parpadeo rápido
    Connecting,   // parpadeo lento
    Operational,  // sólido
    Error,        // parpadeo de error
    Anomaly       // alerta
};

class IIndicator {
public:
    virtual ~IIndicator() = default;
    virtual void show(Status status) = 0;
};

} // namespace glp::domain
