#pragma once

#include <string>

#include "Application/Dto/TelemetrySnapshot.h"

namespace glp::app {

/**
 * Puerto de serialización. La implementación real (Infra) usa ArduinoJson; los
 * tests usan un fake. Así la Application nunca depende de ArduinoJson.
 */
class IPayloadSerializer {
public:
    virtual ~IPayloadSerializer() = default;

    /** Payload de telemetría con el sobre {Device, Timestamp, Details}. */
    virtual std::string telemetry(const TelemetrySnapshot& snap) = 0;
};

} // namespace glp::app
