#pragma once

#include <string>
#include "Application/Dto/EnvelopeContext.h"
#include "Domain/Entities/GpsFix.h"
#include "Domain/Entities/SensorSample.h"
#include "Domain/Entities/SystemMetrics.h"
#include "Domain/Entities/TankDimensions.h"
#include "Domain/Entities/TiltReading.h"
#include "Domain/Entities/VolumeEstimate.h"
#include "Domain/Services/SensorHealth.h"

namespace glp::app {

/**
 * Todo lo necesario para serializar un mensaje de telemetría GLP con el sobre
 * {Device, Timestamp, Details}. El envelope va en `ctx`; el resto es el "Details"
 * específico de la medición de GLP y seguridad del activo estacionario.
 */
struct TelemetrySnapshot {
    EnvelopeContext        ctx;
    domain::SensorSample   raw;       // lecturas crudas
    domain::VolumeEstimate estimate;  // filtradas + volumen + anomalía
    domain::TankDimensions tank;
    domain::GpsFix         gps;
    domain::TiltReading    tilt;      // inclinación e impacto sísmico (GTCRA)
    domain::SystemMetrics  metrics;   // rssi, free_heap, uptime

    // Diagnóstico (Tier 2) y Seguridad Celular
    domain::SensorHealthScores health;
    float consumptionLpd     = 0.0f;  // consumo L/día
    float remainingDays      = 0.0f;  // autonomía estimada
    float vaporPressureBar   = 0.0f;  // Antoine(T̂) esperada, para el residual
    bool  cellularJamming    = false; // detección de inhibidor RF (GTJDC)
    std::string resolvedApn;          // APN resuelto por MCC/MNC o configuración
};

} // namespace glp::app
