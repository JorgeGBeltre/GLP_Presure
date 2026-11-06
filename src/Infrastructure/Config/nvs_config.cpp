#include "Infrastructure/Config/nvs_config.h"
#include <cstdio>
#include <cstring>
#include <esp_system.h>
#include <mbedtls/sha256.h>

AppConfig config;
static Preferences preferences;

static bool g_cellSimPresent = false;
static bool g_cellApnAutoDetected = false;
static bool g_cellDataConnected = false;
static char g_cellResolvedApn[64] = {0};

void setCellularProvisioningSnapshot(bool simPresent, bool apnAutoDetected, bool cellularConnected, const char* resolvedApn) {
    g_cellSimPresent = simPresent;
    g_cellApnAutoDetected = apnAutoDetected;
    g_cellDataConnected = cellularConnected;
    if (resolvedApn && resolvedApn[0] != '\0') {
        std::snprintf(g_cellResolvedApn, sizeof(g_cellResolvedApn), "%s", resolvedApn);
    } else if (!apnAutoDetected) {
        g_cellResolvedApn[0] = '\0';
    }
}

bool isCellularSimPresent() { return g_cellSimPresent; }
bool isCellularApnAutoDetected() { return g_cellApnAutoDetected; }
bool isCellularDataConnected() { return g_cellDataConnected; }
const char* getCellularResolvedApn() { return g_cellResolvedApn; }

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

bool isAccountBound() {
    return config.account_bound;
}

bool hasValidWifiCredentials() {
    return config.wifi_ssid.length() > 0;
}

bool hasValidCellularCredentials() {
    return config.apn.length() > 0 || g_cellApnAutoDetected;
}

bool hasValidBrokerCredentials() {
    return config.mqtt_broker.length() > 0 &&
           config.mqtt_broker != "127.0.0.1" &&
           config.mqtt_user.length() > 0;
}

void computeUnlinkProofHex(const char* deviceKey, const char* chipId, char outHex[33]) {
    outHex[0] = '\0';
    if (!deviceKey || !chipId || deviceKey[0] == '\0' || chipId[0] == '\0') {
        return;
    }
    char inputBuf[192];
    const int n = std::snprintf(inputBuf, sizeof(inputBuf), "%s:UNLINK:%s", deviceKey, chipId);
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(inputBuf)) {
        return;
    }
    uint8_t digest[32] = {0};
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, reinterpret_cast<const unsigned char*>(inputBuf), static_cast<size_t>(n));
    mbedtls_sha256_finish(&ctx, digest);
    mbedtls_sha256_free(&ctx);

    for (size_t i = 0; i < 16; ++i) {
        std::snprintf(outHex + (i * 2), 3, "%02x", digest[i]);
    }
    outHex[32] = '\0';
}

bool verifyUnlinkProof(const char* deviceKey, const char* chipId, const char* providedHex) {
    if (!providedHex || std::strlen(providedHex) != 32) {
        return false;
    }
    char expectedHex[33] = {0};
    computeUnlinkProofHex(deviceKey, chipId, expectedHex);
    if (expectedHex[0] == '\0') {
        return false;
    }
    uint8_t diff = 0;
    for (size_t i = 0; i < 32; ++i) {
        char a = providedHex[i];
        if (a >= 'A' && a <= 'F') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        diff |= static_cast<uint8_t>(a ^ expectedHex[i]);
    }
    return diff == 0;
}

const char* resolveDeviceConfigState(bool wifiConnected, bool cellularConnected, bool mqttConnected) {
    const bool anyNet = wifiConnected || cellularConnected;
    if (!hasValidWifiCredentials() && !hasValidCellularCredentials() && !anyNet) {
        return "unconfigured";
    }
    if (!hasValidBrokerCredentials() && !isAccountBound()) {
        return anyNet ? "wifi_connected" : "incomplete";
    }
    if (anyNet && mqttConnected) {
        return "online";
    }
    if (anyNet) {
        return "wifi_connected";
    }
    if (config.last_wifi_error.length() > 0) {
        return "connection_failed";
    }
    return "configured_disconnected";
}

const char* resolveDeviceCredentialMode(bool wifiConnected) {
    if (!isAccountBound() &&
        (g_cellApnAutoDetected || g_cellDataConnected || wifiConnected || config.apn.length() > 0)) {
        return "adopt_only";
    }
    if (!g_cellApnAutoDetected && config.apn.length() == 0) {
        return "apn";
    }
    return "adopt_only";
}

bool isProvisioned() {
    preferences.begin("glp-config", true);
    const bool ok = preferences.getBool("provisioned", false);
    const bool acct = preferences.getBool("acct_bound", false);
    const String ssid = preferences.getString("wifi_ssid", "");
    const String apn = preferences.getString("apn", "");
    preferences.end();
    return ok && (acct || ssid.length() > 0 || apn.length() > 0);
}

void loadConfig() {
    preferences.begin("glp-config", true);
    config.apn              = preferences.getString("apn",         "");
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
    config.last_wifi_error  = preferences.getString("wifi_err",    "");
    const bool hasAcctBoundKey = preferences.isKey("acct_bound");
    const bool storedAcctBound = preferences.getBool("acct_bound", false);
    preferences.end();

    config.account_bound = hasAcctBoundKey ? storedAcctBound : hasValidBrokerCredentials();

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
    if (hasValidBrokerCredentials() || config.bind_token.length() > 0) {
        config.account_bound = true;
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
    preferences.putBool  ("acct_bound",  config.account_bound);
    preferences.putString("wifi_err",    config.last_wifi_error);
    preferences.putBool  ("provisioned",
                          config.account_bound ||
                          hasValidBrokerCredentials() ||
                          config.wifi_ssid.length() > 0 ||
                          config.apn.length() > 0);
    preferences.end();
}

void resetConfig() {
    preferences.begin("glp-config", false);
    preferences.clear();
    preferences.end();
    config = AppConfig{};
    config.device_id = getDefaultChipId();
    config.device_key = getDefaultDeviceKey();
}

bool saveSledOnlineNvs(bool enabled) {
    config.sled_online = enabled;
    if (!preferences.begin("glp-config", false)) return false;
    preferences.putBool("sled_on", enabled);
    preferences.end();
    return true;
}
