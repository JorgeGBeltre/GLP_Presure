#include "Infrastructure/Config/nvs_config.h"

AppConfig config;
Preferences preferences;

void initConfig() {
    loadConfig();
}

bool isProvisioned() {
    preferences.begin("glp-config", true);
    bool ok = preferences.getBool("provisioned", false);
    preferences.end();
    return ok;
}

void loadConfig() {
    preferences.begin("glp-config", true);
    config.apn = preferences.getString("apn", "internet.claro.com.do");
    config.apn_user = preferences.getString("apn_user", "");
    config.apn_pass = preferences.getString("apn_pass", "");
    config.mqtt_broker = preferences.getString("mqtt_broker", "broker.hivemq.com");
    config.mqtt_port = preferences.getInt("mqtt_port", 1883);
    config.mqtt_user = preferences.getString("mqtt_user", "");
    config.mqtt_pass = preferences.getString("mqtt_pass", "");
    config.wifi_ssid = preferences.getString("wifi_ssid", "");
    config.wifi_pass = preferences.getString("wifi_pass", "");
    config.device_id = preferences.getString("device_id", "GLP-001");
    config.tank_radius_cm = preferences.getFloat("radius", 45.0f);
    config.tank_length_cm = preferences.getFloat("length", 120.0f);
    config.ota_hmac_key = preferences.getString("ota_hmac", "");
    preferences.end();
}

void saveConfig() {
    preferences.begin("glp-config", false);
    preferences.putString("apn", config.apn);
    preferences.putString("apn_user", config.apn_user);
    preferences.putString("apn_pass", config.apn_pass);
    preferences.putString("mqtt_broker", config.mqtt_broker);
    preferences.putInt("mqtt_port", config.mqtt_port);
    preferences.putString("mqtt_user", config.mqtt_user);
    preferences.putString("mqtt_pass", config.mqtt_pass);
    preferences.putString("wifi_ssid", config.wifi_ssid);
    preferences.putString("wifi_pass", config.wifi_pass);
    preferences.putString("device_id", config.device_id);
    preferences.putFloat("radius", config.tank_radius_cm);
    preferences.putFloat("length", config.tank_length_cm);
    preferences.putString("ota_hmac", config.ota_hmac_key);
    preferences.putBool("provisioned", true);
    preferences.end();
}

void resetConfig() {
    preferences.begin("glp-config", false);
    preferences.clear();
    preferences.end();
}
