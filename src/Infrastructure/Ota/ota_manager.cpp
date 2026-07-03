// TINY_GSM_MODEM_SIM7600 defined via build_flags in platformio.ini
#include <TinyGsmClient.h>
#include "MQTTOTAv5.h"

#include "ota_manager.h"
#include "Infrastructure/Config/nvs_config.h"

// mqtt instance is defined in mqtt_client.cpp — declared extern here
// (included via ota_manager.h -> mqtt_client.h chain)
extern ESP32_MQTTv5 mqtt;

static MQTTOTAv5 ota;

// Estado "en progreso": true entre el primer chunk (onProgress) y el desenlace
// (onSuccess/onError). Lo consume Infrastructure/Ota/Mqttv5OtaService.
static volatile bool g_otaInProgress = false;

bool otaInProgress() { return g_otaInProgress; }

void initOTA() {
    // begin() is called inside mqtt.onConnect() — this is the required pattern
    ota.begin(mqtt, config.device_id, "1.0.0");

    // HMAC-SHA256 signature verification
    if (config.ota_hmac_key.length() > 0) {
        ota.setSecurityKey(config.ota_hmac_key.c_str());
        ota.requireSignature(true);
        Serial.println("[OTA] HMAC-SHA256 signature verification ENABLED.");
    } else {
        ota.requireSignature(false);
        Serial.println("[OTA] WARNING: no HMAC key — signature verification DISABLED.");
    }

    ota.onProgress([](int pct, const String& ver) {
        g_otaInProgress = true;
        Serial.printf("[OTA] %s — %d%% complete\n", ver.c_str(), pct);
    });

    ota.onSuccess([](const String& ver) {
        g_otaInProgress = false;
        Serial.printf("[OTA] Firmware v%s installed. Restarting...\n", ver.c_str());
    });

    ota.onError([](const String& err, const String& ver) {
        g_otaInProgress = false;
        Serial.printf("[OTA] ERROR updating to v%s: %s\n", ver.c_str(), err.c_str());
    });

    ota.onStateChange([](uint8_t state) {
        Serial.printf("[OTA] State changed → %d\n", state);
    });

    Serial.println("[OTA] Manager initialized.");
}

void otaHandle() {
    ota.handle();
}
