#pragma once

#include "Application/Dto/EnvelopeContext.h"
#include "Domain/Entities/GpsFix.h"
#include "Domain/Entities/SensorSample.h"
#include "Domain/Entities/SystemMetrics.h"
#include "Domain/Entities/TankDimensions.h"
#include "Domain/Entities/VolumeEstimate.h"

namespace glp::app {

/**
 * Todo lo necesario para serializar un mensaje de telemetría GLP con el sobre
 * {Device, Timestamp, Details}. El envelope va en `ctx`; el resto es el "Details"
 * específico de la medición de GLP.
 */
struct TelemetrySnapshot {
    EnvelopeContext        ctx;
    domain::SensorSample   raw;       // lecturas crudas
    domain::VolumeEstimate estimate;  // filtradas + volumen + anomalía
    domain::TankDimensions tank;
    domain::GpsFix         gps;
    domain::SystemMetrics  metrics;   // rssi, free_heap, uptime
};

} // namespace glp::app
