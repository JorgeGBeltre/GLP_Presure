#pragma once

#include "Application/Serialization/IPayloadSerializer.h"

namespace glp::infra {

/** IPayloadSerializer sobre ArduinoJson — emite el sobre {Device, Timestamp, Details}. */
class ArduinoJsonPayloadSerializer : public app::IPayloadSerializer {
public:
    std::string telemetry(const app::TelemetrySnapshot& snap) override;
};

} // namespace glp::infra
