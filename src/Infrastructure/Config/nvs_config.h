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
    String ota_hmac_key;
};

extern AppConfig config;

void initConfig();
bool isProvisioned();
void saveConfig();
void loadConfig();
void resetConfig();
