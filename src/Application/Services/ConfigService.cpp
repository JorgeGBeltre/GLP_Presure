#include "Application/Services/ConfigService.h"

namespace glp::app {

bool ConfigService::load() {
    const bool ok = repo_.load(config_);
    config_.provisioned = repo_.isProvisioned();
    return ok;
}

bool ConfigService::applyRemoteUpdate(const RemoteConfigUpdate& update) {
    bool changed = false;

    if (update.tankRadiusCm) { config_.tankRadiusCm = *update.tankRadiusCm; changed = true; }
    if (update.tankLengthCm) { config_.tankLengthCm = *update.tankLengthCm; changed = true; }
    if (update.mqttBroker)   { config_.mqttBroker   = *update.mqttBroker;   changed = true; }
    if (update.mqttPort)     { config_.mqttPort     = *update.mqttPort;     changed = true; }
    if (update.otaHmacKey)   { config_.otaHmacKey   = *update.otaHmacKey;   changed = true; }

    if (!changed) return false;
    return repo_.save(config_);
}

} // namespace glp::app
