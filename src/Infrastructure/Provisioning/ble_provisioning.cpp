#include "ble_provisioning.h"
#include "Infrastructure/Config/nvs_config.h"
#include <NimBLEDevice.h>

// UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHAR_APN_UUID       "aaaaaaaa-bbbb-cccc-dddd-000000000001"
#define CHAR_APN_USER_UUID  "aaaaaaaa-bbbb-cccc-dddd-000000000002"
#define CHAR_APN_PASS_UUID  "aaaaaaaa-bbbb-cccc-dddd-000000000003"
#define CHAR_BROKER_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000004"
#define CHAR_PORT_UUID      "aaaaaaaa-bbbb-cccc-dddd-000000000005"
#define CHAR_MQTTUSER_UUID  "aaaaaaaa-bbbb-cccc-dddd-000000000006"
#define CHAR_MQTTPASS_UUID  "aaaaaaaa-bbbb-cccc-dddd-000000000007"
#define CHAR_DEVID_UUID     "aaaaaaaa-bbbb-cccc-dddd-000000000008"
#define CHAR_RADIUS_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000009"
#define CHAR_LENGTH_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000010"
#define CHAR_OTAKEY_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000011"
#define CHAR_WIFISSID_UUID  "aaaaaaaa-bbbb-cccc-dddd-000000000012"
#define CHAR_WIFIPASS_UUID  "aaaaaaaa-bbbb-cccc-dddd-000000000013"
#define CHAR_SAVE_UUID      "aaaaaaaa-bbbb-cccc-dddd-000000000099"

static bool _configSaved = false;

// ---- Generic write callback helper ----
class StringWriteCallback : public NimBLECharacteristicCallbacks {
public:
    String* _target;
    explicit StringWriteCallback(String* target) : _target(target) {}
    void onWrite(NimBLECharacteristic* c) override {
        *_target = c->getValue().c_str();
        Serial.printf("[BLE] %s = %s\n", c->getUUID().toString().c_str(), _target->c_str());
    }
};

class SaveCallback : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* c) override {
        saveConfig();
        _configSaved = true;
        Serial.println("[BLE] Config saved to NVS. Sending ACK...");
        c->setValue("OK");
        c->notify();
    }
};

void initBLEProvisioning() {
    // init happens inside runBLEProvisioning
}

bool runBLEProvisioning(unsigned long timeoutMs) {
    Serial.println("[BLE] Starting provisioning server...");
    _configSaved = false;

    NimBLEDevice::init(("GLP-" + config.device_id).c_str());
    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* service = server->createService(SERVICE_UUID);

    auto mkChar = [&](const char* uuid, String* field) {
        NimBLECharacteristic* ch = service->createCharacteristic(
            uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
        ch->setCallbacks(new StringWriteCallback(field));
        return ch;
    };

    mkChar(CHAR_APN_UUID,      &config.apn);
    mkChar(CHAR_APN_USER_UUID, &config.apn_user);
    mkChar(CHAR_APN_PASS_UUID, &config.apn_pass);
    mkChar(CHAR_BROKER_UUID,   &config.mqtt_broker);
    mkChar(CHAR_MQTTUSER_UUID, &config.mqtt_user);
    mkChar(CHAR_MQTTPASS_UUID, &config.mqtt_pass);
    mkChar(CHAR_DEVID_UUID,    &config.device_id);
    mkChar(CHAR_OTAKEY_UUID,   &config.ota_hmac_key);
    mkChar(CHAR_WIFISSID_UUID, &config.wifi_ssid);
    mkChar(CHAR_WIFIPASS_UUID, &config.wifi_pass);

    // Port char (int stored as string)
    NimBLECharacteristic* portChar = service->createCharacteristic(
        CHAR_PORT_UUID, NIMBLE_PROPERTY::WRITE);
    portChar->setCallbacks(new NimBLECharacteristicCallbacks() /* handled inline below */);

    // Radius / Length (float stored as string)
    NimBLECharacteristic* radiusChar = service->createCharacteristic(
        CHAR_RADIUS_UUID, NIMBLE_PROPERTY::WRITE);
    NimBLECharacteristic* lengthChar = service->createCharacteristic(
        CHAR_LENGTH_UUID, NIMBLE_PROPERTY::WRITE);

    // Save char
    NimBLECharacteristic* saveChar = service->createCharacteristic(
        CHAR_SAVE_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    saveChar->setCallbacks(new SaveCallback());

    service->start();

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(SERVICE_UUID);
    adv->start();

    Serial.printf("[BLE] Advertising as 'GLP-%s' for %lu ms\n",
                  config.device_id.c_str(), timeoutMs);

    unsigned long start = millis();
    while ((millis() - start) < timeoutMs) {
        if (_configSaved) {
            Serial.println("[BLE] Provisioning complete!");
            adv->stop();
            NimBLEDevice::deinit(true);
            return true;
        }
        delay(100);
    }

    Serial.println("[BLE] Timeout — no config received via BLE.");
    adv->stop();
    NimBLEDevice::deinit(true);
    return false;
}
