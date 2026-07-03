#pragma once

namespace glp::domain {

/** Servicio de actualización OTA (MQTTOTAv5 en el firmware real). */
class IOtaService {
public:
    virtual ~IOtaService() = default;

    virtual void begin() = 0;

    /** Maneja timeouts / reinicio no bloqueante. Llamar cada loop. */
    virtual void handle() = 0;

    /** true mientras una descarga/flasheo OTA está en curso. */
    virtual bool isInProgress() = 0;
};

} // namespace glp::domain
