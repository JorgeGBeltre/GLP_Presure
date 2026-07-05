#include "Application/Services/TelemetryService.h"

#include "Application/Dto/TelemetrySnapshot.h"
#include "Domain/Services/SensorHealth.h"

namespace glp::app {

bool TelemetryService::suppressed() {
    return !conn_.transport().isConnected() || ota_.isInProgress();
}

void TelemetryService::loop() {
    const unsigned long now = clock_.nowMs();
    if (!seeded_) {                 // sembrar el temporizador en el primer tick
        seeded_ = true;
        lastPublishMs_ = now;
        return;
    }
    if (now - lastPublishMs_ >= intervalMs_) {
        lastPublishMs_ = now;
        publishTelemetry();
    }
}

bool TelemetryService::publishTelemetry() {
    if (suppressed()) return false;

    gps_.feed();
    const domain::SensorSample   raw  = sensors_.read();
    const domain::TankDimensions tank = config_.tankDimensions();
    const domain::VolumeEstimate est  = fusion_.process(raw, tank, config_.propaneFraction());
    const domain::GpsFix         fix  = gps_.location();

    // Diagnóstico (Tier 2): consumo, salud por sensor y presión de vapor esperada.
    const float frac = config_.propaneFraction();
    consumption_.update(est.volumeLiters, clock_.nowMs());

    TelemetrySnapshot snap;
    // --- Envelope (cabecera "Device" + "Timestamp") ---
    snap.ctx.identity         = device_.identity();
    snap.ctx.identity.deviceId = config_.config().deviceId;
    snap.ctx.deviceId         = config_.config().deviceId;
    snap.ctx.connectionType   = conn_.activeTypeString();
    snap.ctx.ipAddress        = conn_.activeIp();
    snap.ctx.timestamp        = time_.isoTimestamp();
    // --- Details (medición GLP) ---
    snap.raw      = raw;
    snap.estimate = est;
    snap.tank     = tank;
    snap.gps      = fix;
    snap.metrics  = device_.metrics();
    snap.metrics.rssi = conn_.activeRssi();   // rssi del enlace activo
    snap.health           = domain::SensorHealth::evaluate(est, raw, frac);
    snap.consumptionLpd   = consumption_.litersPerDay();
    snap.remainingDays    = consumption_.remainingDays(est.volumeLiters);
    snap.vaporPressureBar = domain::LpgThermo::vaporPressureBar(est.tempCelsius, frac);

    return conn_.transport().publish(topics_.telemetry, serializer_.telemetry(snap));
}

} // namespace glp::app
