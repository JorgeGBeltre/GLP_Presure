#pragma once

#include "Domain/Interfaces/IOtaService.h"
#include "Infrastructure/Ota/ota_manager.h"   // initOTA (llamado desde mqtt.onConnect), otaHandle

namespace glp::infra {

/**
 * IOtaService sobre MQTTOTAv5 (driver ota/ota_manager).
 * isInProgress() refleja el estado real: el driver activa un flag en onProgress
 * y lo baja en onSuccess/onError.
 */
class Mqttv5OtaService : public domain::IOtaService {
public:
    void begin() override { initOTA(); }
    void handle() override { otaHandle(); }
    bool isInProgress() override { return otaInProgress(); }
};

} // namespace glp::infra
