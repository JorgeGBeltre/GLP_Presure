#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct AppConfig {
    String apn;
    String apn_user;
    String apn_pass;
    String mqtt_broker;
    int mqtt_port;
    String mqtt_user;
    String mqtt_pass;
    String wifi_ssid;
    String wifi_pass;
    String device_id;
    float tank_radius_cm;
    float tank_length_cm;
    float tank_height_cm;
    float mount_offset_cm;
    float press_slope;
    float press_offset;
    float temp_r_ref;
    float temp_v_exc;
    float temp_offset_c;
    String ota_hmac_key;
};

extern AppConfig config;

void initConfig();
bool isProvisioned();
void saveConfig();
void loadConfig();
void resetConfig();
