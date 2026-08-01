#include "Infrastructure/Serialization/ArduinoJsonPayloadSerializer.h"

#include <ArduinoJson.h>

namespace glp::infra {

std::string ArduinoJsonPayloadSerializer::telemetry(const app::TelemetrySnapshot& s) {
    DynamicJsonDocument doc(2560);

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
    gps["lat"]   = s.gps.latitude;
    gps["lng"]   = s.gps.longitude;
    gps["valid"] = s.gps.valid;

    d["rssi"]      = s.metrics.rssi;
    d["free_heap"] = s.metrics.freeHeap;
    d["uptime"]    = s.metrics.uptime;

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

    String out;
    serializeJson(doc, out);
    return out.c_str();
}

} // namespace glp::infra
