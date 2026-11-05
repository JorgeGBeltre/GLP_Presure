#pragma once
#include <Arduino.h>
#include <Preferences.h>

constexpr const char* GLP_FIRMWARE_VERSION = "v1.0.1";

struct AppConfig {
    String apn;
    String apn_user;
    String apn_pass;
    String mqtt_broker;
    int    mqtt_port = 8883;
    String mqtt_user;
    String mqtt_pass;
    String wifi_ssid;
    String wifi_pass;
    String device_id;
    String device_key;
    String bind_token;
    float  tank_radius_cm = 50.0f;
    float  tank_length_cm = 200.0f;
    float  tank_height_cm = 100.0f;
    float  mount_offset_cm = 0.0f;
    float  press_slope = 1.0f;
    float  press_offset = 0.0f;
    float  temp_r_ref = 10000.0f;
    float  temp_v_exc = 3.3f;
    float  temp_offset_c = 0.0f;
    float  propane_fraction = 0.60f;
    float  sensor_axial_cm = 0.0f;
    float  ref_distance_cm = 0.0f;
    String ota_hmac_key;
    bool   sled_online = true;
    bool   account_bound = false;
    String last_wifi_error;
};

extern AppConfig config;

void initConfig();
bool isProvisioned();
bool isAccountBound();
bool hasValidWifiCredentials();
bool hasValidCellularCredentials();
bool hasValidBrokerCredentials();
void computeUnlinkProofHex(const char* deviceKey, const char* chipId, char outHex[33]);
bool verifyUnlinkProof(const char* deviceKey, const char* chipId, const char* providedHex);
const char* resolveDeviceConfigState(bool wifiConnected, bool cellularConnected, bool mqttConnected);
const char* resolveDeviceCredentialMode(bool wifiConnected);

void setCellularProvisioningSnapshot(bool simPresent, bool apnAutoDetected, bool cellularConnected, const char* resolvedApn);
bool isCellularSimPresent();
bool isCellularApnAutoDetected();
bool isCellularDataConnected();
const char* getCellularResolvedApn();

void saveConfig();
void loadConfig();
void resetConfig();
bool saveSledOnlineNvs(bool enabled);
