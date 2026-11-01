#include "Infrastructure/Config/nvs_config.h"
#include <esp_system.h>

AppConfig config;
static Preferences preferences;

static String getDefaultChipId() {
    uint64_t chipId = ESP.getEfuseMac();
    char buf[16];
    snprintf(buf, sizeof(buf), "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
    return String(buf);
}

static String getDefaultDeviceKey() {
    uint64_t mac = ESP.getEfuseMac();
    char buf[37];
    snprintf(buf, sizeof(buf), "%08X-%04X-4%03X-%04X-%012llX",
             (uint32_t)(mac & 0xFFFFFFFF),
             (uint16_t)((mac >> 32) & 0xFFFF),
             (uint16_t)((mac >> 16) & 0x0FFF),
             (uint16_t)(((mac >> 48) & 0x3FFF) | 0x8000),
             (unsigned long long)(mac ^ 0x5A5A5A5A5A5AULL));
    return String(buf);
}

void initConfig() {
    loadConfig();
    if (config.device_id.length() == 0) {
        config.device_id = getDefaultChipId();
    }
    if (config.device_key.length() == 0) {
        config.device_key = getDefaultDeviceKey();
    }
}

bool isProvisioned() {
    preferences.begin("glp-config", true);
    bool ok = preferences.getBool("provisioned", false);
    preferences.end();
    return ok;
}

void loadConfig() {
    preferences.begin("glp-config", true);
    config.apn              = preferences.getString("apn",         "internet.claro.com.do");
    config.apn_user         = preferences.getString("apn_user",    "");
    config.apn_pass         = preferences.getString("apn_pass",    "");
    config.mqtt_broker      = preferences.getString("mqtt_broker", "");
    config.mqtt_port        = preferences.getInt   ("mqtt_port",   8883);
    config.mqtt_user        = preferences.getString("mqtt_user",   "");
    config.mqtt_pass        = preferences.getString("mqtt_pass",   "");
    config.wifi_ssid        = preferences.getString("wifi_ssid",   "");
    config.wifi_pass        = preferences.getString("wifi_pass",   "");
    config.device_id        = preferences.getString("device_id",   "");
    config.device_key       = preferences.getString("device_key",  "");
    config.bind_token       = preferences.getString("bind_token",  "");
    config.tank_radius_cm   = preferences.getFloat ("tank_r",      50.0f);
    config.tank_length_cm   = preferences.getFloat ("tank_l",      200.0f);
    config.tank_height_cm   = preferences.getFloat ("tank_h",      2.0f * config.tank_radius_cm);
    config.mount_offset_cm  = preferences.getFloat ("mount_off",   0.0f);
    config.press_slope      = preferences.getFloat ("p_slope",     1.0f);
    config.press_offset     = preferences.getFloat ("p_offset",    0.0f);
    config.temp_r_ref       = preferences.getFloat ("t_rref",      10000.0f);
    config.temp_v_exc       = preferences.getFloat ("t_vexc",      3.3f);
    config.temp_offset_c    = preferences.getFloat ("t_offset",    0.0f);
    config.propane_fraction = preferences.getFloat ("prop_frac",   0.60f);
    config.sensor_axial_cm  = preferences.getFloat ("s_axial",     0.0f);
    config.ref_distance_cm  = preferences.getFloat ("ref_dist",    0.0f);
    config.ota_hmac_key     = preferences.getString("ota_hmac",    "");
    config.sled_online      = preferences.getBool  ("sled_on",     true);
    preferences.end();

    if (config.device_id.length() == 0) {
        config.device_id = getDefaultChipId();
    }
    if (config.device_key.length() == 0) {
        config.device_key = getDefaultDeviceKey();
    }
}

void saveConfig() {
    if (config.device_id.length() == 0) {
        config.device_id = getDefaultChipId();
    }
    if (config.device_key.length() == 0) {
        config.device_key = getDefaultDeviceKey();
    }
    preferences.begin("glp-config", false);
    preferences.putString("apn",         config.apn);
    preferences.putString("apn_user",    config.apn_user);
    preferences.putString("apn_pass",    config.apn_pass);
    preferences.putString("mqtt_broker", config.mqtt_broker);
    preferences.putInt   ("mqtt_port",   config.mqtt_port);
    preferences.putString("mqtt_user",   config.mqtt_user);
    preferences.putString("mqtt_pass",   config.mqtt_pass);
    preferences.putString("wifi_ssid",   config.wifi_ssid);
    preferences.putString("wifi_pass",   config.wifi_pass);
    preferences.putString("device_id",   config.device_id);
    preferences.putString("device_key",  config.device_key);
    preferences.putString("bind_token",  config.bind_token);
    preferences.putFloat ("tank_r",      config.tank_radius_cm);
    preferences.putFloat ("tank_l",      config.tank_length_cm);
    preferences.putFloat ("tank_h",      config.tank_height_cm);
    preferences.putFloat ("mount_off",   config.mount_offset_cm);
    preferences.putFloat ("p_slope",     config.press_slope);
    preferences.putFloat ("p_offset",    config.press_offset);
    preferences.putFloat ("t_rref",      config.temp_r_ref);
    preferences.putFloat ("t_vexc",      config.temp_v_exc);
    preferences.putFloat ("t_offset",    config.temp_offset_c);
    preferences.putFloat ("prop_frac",   config.propane_fraction);
    preferences.putFloat ("s_axial",     config.sensor_axial_cm);
    preferences.putFloat ("ref_dist",    config.ref_distance_cm);
    preferences.putString("ota_hmac",    config.ota_hmac_key);
    preferences.putBool  ("sled_on",     config.sled_online);
    preferences.putBool  ("provisioned", true);
    preferences.end();
}

void resetConfig() {
    preferences.begin("glp-config", false);
    preferences.clear();
    preferences.end();
}

bool saveSledOnlineNvs(bool enabled) {
    config.sled_online = enabled;
    if (!preferences.begin("glp-config", false)) return false;
    preferences.putBool("sled_on", enabled);
    preferences.end();
    return true;
}
