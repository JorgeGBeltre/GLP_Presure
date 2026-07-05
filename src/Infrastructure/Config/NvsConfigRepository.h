#pragma once

#include "Domain/Interfaces/IConfigRepository.h"
#include "Infrastructure/Config/nvs_config.h"  // extern AppConfig config; initConfig/loadConfig/saveConfig/isProvisioned/resetConfig

namespace glp::infra {

/**
 * IConfigRepository sobre el driver NVS existente (provisioning/nvs_config).
 * Hace de puente entre el `AppConfig` antiguo (Arduino String, global) y la
 * entidad de dominio pura `DeviceConfig` (std::string).
 */
class NvsConfigRepository : public domain::IConfigRepository {
public:
    /** Abre el namespace NVS. Llamar una vez en setup(). */
    void begin() { initConfig(); }

    bool load(domain::DeviceConfig& out) override {
        loadConfig();
        out = toDomain(::config);
        out.provisioned = ::isProvisioned();
        return out.provisioned;
    }

    bool save(const domain::DeviceConfig& cfg) override {
        ::config = fromDomain(cfg);
        saveConfig();
        return true;
    }

    bool erase() override { resetConfig(); return true; }
    bool isProvisioned() override { return ::isProvisioned(); }

private:
    static domain::DeviceConfig toDomain(const AppConfig& c) {
        domain::DeviceConfig d;
        d.apn         = c.apn.c_str();
        d.apnUser     = c.apn_user.c_str();
        d.apnPass     = c.apn_pass.c_str();
        d.mqttBroker  = c.mqtt_broker.c_str();
        d.mqttPort    = c.mqtt_port;
        d.mqttUser    = c.mqtt_user.c_str();
        d.mqttPass    = c.mqtt_pass.c_str();
        d.wifiSsid    = c.wifi_ssid.c_str();
        d.wifiPass    = c.wifi_pass.c_str();
        d.deviceId    = c.device_id.c_str();
        d.tankRadiusCm = c.tank_radius_cm;
        d.tankLengthCm = c.tank_length_cm;
        d.tankHeightCm  = c.tank_height_cm;
        d.mountOffsetCm = c.mount_offset_cm;
        d.calibration.pressSlope  = c.press_slope;
        d.calibration.pressOffset = c.press_offset;
        d.calibration.tempRRef    = c.temp_r_ref;
        d.calibration.tempVExc    = c.temp_v_exc;
        d.calibration.tempOffsetC = c.temp_offset_c;
        d.otaHmacKey  = c.ota_hmac_key.c_str();
        return d;
    }

    static AppConfig fromDomain(const domain::DeviceConfig& d) {
        AppConfig c;
        c.apn         = d.apn.c_str();
        c.apn_user    = d.apnUser.c_str();
        c.apn_pass    = d.apnPass.c_str();
        c.mqtt_broker = d.mqttBroker.c_str();
        c.mqtt_port   = d.mqttPort;
        c.mqtt_user   = d.mqttUser.c_str();
        c.mqtt_pass   = d.mqttPass.c_str();
        c.wifi_ssid   = d.wifiSsid.c_str();
        c.wifi_pass   = d.wifiPass.c_str();
        c.device_id   = d.deviceId.c_str();
        c.tank_radius_cm = d.tankRadiusCm;
        c.tank_length_cm = d.tankLengthCm;
        c.tank_height_cm  = d.tankHeightCm;
        c.mount_offset_cm = d.mountOffsetCm;
        c.press_slope  = d.calibration.pressSlope;
        c.press_offset = d.calibration.pressOffset;
        c.temp_r_ref   = d.calibration.tempRRef;
        c.temp_v_exc   = d.calibration.tempVExc;
        c.temp_offset_c = d.calibration.tempOffsetC;
        c.ota_hmac_key = d.otaHmacKey.c_str();
        return c;
    }
};

} // namespace glp::infra
