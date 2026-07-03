#pragma once

#include "Domain/Entities/DeviceIdentity.h"
#include "Domain/Entities/SystemMetrics.h"

namespace glp::domain {

/** Identidad y métricas del dispositivo (implementado sobre ESP.* + SIM). */
class IDeviceRepository {
public:
    virtual ~IDeviceRepository() = default;
    virtual DeviceIdentity identity() = 0;
    virtual SystemMetrics  metrics() = 0;
};

} // namespace glp::domain
