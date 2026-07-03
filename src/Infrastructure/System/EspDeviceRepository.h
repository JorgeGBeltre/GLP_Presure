#pragma once

#include "Domain/Interfaces/IDeviceRepository.h"

namespace glp::infra {

/** IDeviceRepository sobre las APIs del ESP32 (eFuse MAC, chip model, heap). */
class EspDeviceRepository : public domain::IDeviceRepository {
public:
    domain::DeviceIdentity identity() override;
    domain::SystemMetrics  metrics() override;
};

} // namespace glp::infra
