#include "Infrastructure/Serialization/ArduinoJsonPayloadSerializer.h"

#include <ArduinoJson.h>

namespace glp::infra {

std::string ArduinoJsonPayloadSerializer::telemetry(const app::TelemetrySnapshot& s) {
    DynamicJsonDocument doc(3072);

    // ── Cabecera común "Device" ──────────────────────────────────
    JsonObject dev = doc.createNestedObject("Device");
    dev["ChipId"]          = s.ctx.identity.chipId;
    dev["ChipType"]        = s.ctx.identity.chipType;
    dev["MacAddress"]      = s.ctx.identity.macAddress;
    dev["FirmwareVersion"] = s.ctx.identity.firmwareVersion;
    dev["DeviceId"]        = s.ctx.deviceId;
    dev["IMEI"]            = s.ctx.identity.imei;
    dev["IMSI"]            = s.ctx.identity.imsi;
    dev["status"]          = "connected";
    dev["TypeConexion"]    = s.ctx.connectionType;
    dev["IPAddress"]       = s.ctx.ipAddress;

    doc["Timestamp"] = s.ctx.timestamp;

    // ── "Details": medición GLP ──────────────────────────────────
    JsonObject d = doc.createNestedObject("Details");

    JsonObject raw = d.createNestedObject("sensors_raw");
    raw["level_mm"]     = s.raw.levelRawMm;
    raw["pressure_bar"] = s.raw.pressureRawBar;
    raw["temp_celsius"] = s.raw.tempRawC;

    JsonObject filt = d.createNestedObject("sensors_filtered");
    filt["level_mm"]              = s.estimate.levelMm;
    filt["pressure_bar"]          = s.estimate.pressureBar;
    filt["temp_celsius"]          = s.estimate.tempCelsius;
    filt["kalman_uncertainty_mm"] = s.estimate.kalmanUncertaintyMm;

    JsonObject vol = d.createNestedObject("volume");
    vol["liters"]       = s.estimate.volumeLiters;
    vol["gallons"]      = s.estimate.gallons;
    vol["percentage"]   = s.estimate.percentage;
    vol["density_kg_l"] = s.estimate.densityKgL;
    vol["mass_kg"]      = s.estimate.massKg;
    vol["liters_15c"]   = s.estimate.volume15Liters;
    vol["anomaly"]      = s.estimate.anomaly;

    JsonObject tank = d.createNestedObject("tank");
    tank["radius_cm"] = s.tank.radiusMm / 10.0f;
    tank["length_cm"] = s.tank.lengthMm / 10.0f;

    JsonObject gps = d.createNestedObject("gps");
    gps["lat"]                = s.gps.latitude;
    gps["lng"]                = s.gps.longitude;
    gps["valid"]              = s.gps.valid;
    gps["positioning_source"] = s.gps.valid
                                    ? (s.gps.isLbsFallback ? "LBS" : "GPS")
                                    : (s.gps.lbsValid ? "LBS_CELL_ID" : "NONE");
    gps["lbs_fallback"]       = s.gps.isLbsFallback;
    gps["drift_filtered"]     = s.gps.driftFiltered;
    gps["displacement_m"]     = s.gps.displacementM;
    gps["tamper_moved"]       = s.gps.tamperMoved;
    if (s.gps.lbsValid) {
        JsonObject lbs = gps.createNestedObject("lbs");
        lbs["mcc"]     = s.gps.mcc;
        lbs["mnc"]     = s.gps.mnc;
        lbs["lac"]     = s.gps.lac;
        lbs["cell_id"] = s.gps.cellId;
        if (s.gps.lbsAccuracyM > 0) {
            lbs["accuracy_m"] = s.gps.lbsAccuracyM;
        }
        char cellKey[48];
        snprintf(cellKey, sizeof(cellKey), "%d-%d-%u-%u",
                 s.gps.mcc, s.gps.mnc, (unsigned)s.gps.lac, (unsigned)s.gps.cellId);
        lbs["cell_key"] = cellKey;
    }

    d["rssi"]               = s.metrics.rssi;
    d["free_heap"]          = s.metrics.freeHeap;
    d["uptime"]             = s.metrics.uptime;
    d["boot_count"]         = s.metrics.bootCount;
    d["unexpected_reboots"] = s.metrics.unexpectedReboots;
    d["wdt_offences"]       = s.metrics.wdtOffences;
    d["modem_recoveries"]   = s.metrics.modemRecoveries;
    d["offline_queued"]     = s.metrics.queuedMessages;
    if (!s.metrics.resetReason.empty()) {
        d["reset_reason"]   = s.metrics.resetReason;
    }

    JsonObject health = d.createNestedObject("health");
    health["ultrasonic"]  = s.health.ultrasonic;
    health["pressure"]    = s.health.pressure;
    health["temperature"] = s.health.temperature;

    JsonObject vapor = d.createNestedObject("vapor_check");
    vapor["p_measured"] = s.estimate.pressureBar;
    vapor["p_antoine"]  = s.vaporPressureBar;
    vapor["residual"]   = s.estimate.pressureBar - s.vaporPressureBar;

    JsonObject cons = d.createNestedObject("consumption");
    cons["l_per_day"]      = s.consumptionLpd;
    cons["remaining_days"] = s.remainingDays;

    JsonObject acoustic = d.createNestedObject("acoustic");
    acoustic["c_used_ms"] = s.raw.soundSpeedMs;
    acoustic["source"]    = s.raw.reflectorUsed ? "reflector" : "gas_model";
    acoustic["healthy"]   = s.raw.acousticHealthy;

    JsonObject security = d.createNestedObject("security_guard");
    security["tamper_moved"]     = s.gps.tamperMoved;
    security["displacement_m"]   = s.gps.displacementM;
    security["seismic_impact"]   = s.tilt.seismicImpactDetected;
    security["accel_g"]          = s.tilt.accelMagnitudeG;
    security["cellular_jamming"] = s.cellularJamming;
    if (!s.resolvedApn.empty()) {
        security["resolved_apn"] = s.resolvedApn;
    }

    String out;
    serializeJson(doc, out);
    return out.c_str();
}

} // namespace glp::infra
