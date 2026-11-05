#include "Infrastructure/Provisioning/ble_provisioning.h"
#include "Infrastructure/Config/nvs_config.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstring>
#include <sstream>

#define SERVICE_UUID          "12345678-1234-1234-1234-123456789abc"
#define CHAR_APN_UUID         "aaaaaaaa-bbbb-cccc-dddd-000000000001"
#define CHAR_APNUSER_UUID     "aaaaaaaa-bbbb-cccc-dddd-000000000002"
#define CHAR_APNPASS_UUID     "aaaaaaaa-bbbb-cccc-dddd-000000000003"
#define CHAR_BROKER_UUID      "aaaaaaaa-bbbb-cccc-dddd-000000000004"
#define CHAR_PORT_UUID        "aaaaaaaa-bbbb-cccc-dddd-000000000005"
#define CHAR_MQTTUSER_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000006"
#define CHAR_MQTTPASS_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000007"
#define CHAR_DEVID_UUID       "aaaaaaaa-bbbb-cccc-dddd-000000000008"
#define CHAR_RADIUS_UUID      "aaaaaaaa-bbbb-cccc-dddd-000000000009"
#define CHAR_LENGTH_UUID      "aaaaaaaa-bbbb-cccc-dddd-000000000010"
#define CHAR_OTAKEY_UUID      "aaaaaaaa-bbbb-cccc-dddd-000000000011"
#define CHAR_WIFISSID_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000012"
#define CHAR_WIFIPASS_UUID    "aaaaaaaa-bbbb-cccc-dddd-000000000013"
#define CHAR_FRAME_JSON_UUID  "aaaaaaaa-bbbb-cccc-dddd-000000000014"
#define CHAR_DEVKEY_UUID      "aaaaaaaa-bbbb-cccc-dddd-000000000015"
#define CHAR_BINDTOKEN_UUID   "aaaaaaaa-bbbb-cccc-dddd-000000000016"
#define CHAR_SAVE_UUID        "aaaaaaaa-bbbb-cccc-dddd-000000000099"

static volatile bool _configSaved = false;
static volatile bool _bleStopping = false;
static std::string _rxFrameBuffer;
static unsigned long _lastRxChunkMs = 0;

static uint8_t computeAdvStateByte() {
    const bool wifiConn = (WiFi.status() == WL_CONNECTED);
    const bool cellConn = isCellularDataConnected();
    const bool anyConn  = wifiConn || cellConn;
    if (!hasValidWifiCredentials() && !hasValidCellularCredentials() && !anyConn) {
        return 0x00; // unconfigured
    }
    if (!hasValidBrokerCredentials() && !isAccountBound()) {
        return anyConn ? 0x04 : 0x01;
    }
    if (anyConn) return 0x04;
    if (config.last_wifi_error.length() > 0) return 0x03;
    return 0x02;
}

static uint8_t computeAdvProvFlags() {
    const bool wifiConn = (WiFi.status() == WL_CONNECTED);
    const char* credMode = resolveDeviceCredentialMode(wifiConn);
    uint8_t credBits = 0x01U; // default: apn
    if (std::strcmp(credMode, "adopt_only") == 0) {
        credBits = 0x02U;
    } else if (std::strcmp(credMode, "wifi") == 0) {
        credBits = 0x00U;
    } else if (std::strcmp(credMode, "wifi_and_apn") == 0) {
        credBits = 0x03U;
    }

    uint8_t flags = credBits;
    if (isCellularApnAutoDetected()) flags |= 0x04U; // bit 2: apn_auto_detected
    if (isCellularDataConnected())   flags |= 0x08U; // bit 3: cellular_connected
    flags |= 0x10U;                                  // bit 4: supports_wifi
    if (isAccountBound())            flags |= 0x20U; // bit 5: account_bound
    return flags;
}

static void buildBleStateJson(String& out) {
    const bool wifiConn = (WiFi.status() == WL_CONNECTED);
    const bool cellConn = isCellularDataConnected();
    const bool autoApn  = isCellularApnAutoDetected();
    const char* credMode = resolveDeviceCredentialMode(wifiConn);
    const bool adoptReady = (std::strcmp(credMode, "adopt_only") == 0);
    const String effectiveApn = config.apn.length() > 0 ? config.apn : String(getCellularResolvedApn());

    StaticJsonDocument<768> idDoc;
    idDoc["device_family"]          = "glp_pressure";
    idDoc["device_code"]            = 4;
    idDoc["device_type"]            = "GLP-Sensor";
    idDoc["model"]                  = "VESIS-GLP-S3";
    idDoc["channel_count"]          = 1;
    idDoc["device_id"]              = config.device_id;
    idDoc["chip_id"]                = config.device_id;
    idDoc["device_key"]             = config.device_key;
    idDoc["mac"]                    = WiFi.macAddress();
    idDoc["version"]                = GLP_FIRMWARE_VERSION;
    idDoc["config_state"]           = resolveDeviceConfigState(wifiConn, cellConn, false);
    idDoc["credential_mode"]        = credMode;
    idDoc["requires_wifi"]          = (std::strcmp(credMode, "wifi") == 0 || std::strcmp(credMode, "wifi_and_apn") == 0);
    idDoc["requires_apn"]           = (std::strcmp(credMode, "apn") == 0 || std::strcmp(credMode, "wifi_and_apn") == 0);
    idDoc["supports_wifi"]          = true;
    idDoc["supports_apn"]           = true;
    idDoc["sim_present"]            = isCellularSimPresent();
    idDoc["apn_auto_detected"]      = autoApn;
    idDoc["cellular_connected"]     = cellConn;
    idDoc["adopt_ready"]            = adoptReady;
    idDoc["apn"]                    = effectiveApn;
    idDoc["wifi_ssid"]              = config.wifi_ssid;
    idDoc["configured_ssid"]        = config.wifi_ssid;
    idDoc["wifi_connected"]         = wifiConn;
    idDoc["has_wifi_credentials"]   = hasValidWifiCredentials();
    idDoc["has_broker_credentials"] = hasValidBrokerCredentials();
    idDoc["account_bound"]          = isAccountBound();
    idDoc["provisioning_mode"]      = true;
    idDoc["server"]                 = config.mqtt_broker;
    idDoc["port"]                   = config.mqtt_port;
    idDoc["tank_radius_cm"]         = config.tank_radius_cm;
    idDoc["tank_length_cm"]         = config.tank_length_cm;
    idDoc["sledOnline"]             = config.sled_online ? "on" : "off";
    idDoc["status"]                 = _configSaved ? "configured" : "ready_to_claim";
    JsonArray methods = idDoc.createNestedArray("methods");
    methods.add("ble");
    methods.add("softap");
    methods.add("smartconfig");
    methods.add("lan");

    out = "";
    serializeJson(idDoc, out);
}

static bool isCompleteJsonObject(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || s[first] != '{') return false;
    int depth = 0;
    bool inStr = false;
    bool esc = false;
    for (size_t i = first; i < s.size(); ++i) {
        char ch = s[i];
        if (inStr) {
            if (esc) esc = false;
            else if (ch == '\\') esc = true;
            else if (ch == '"') inStr = false;
        } else {
            if (ch == '"') inStr = true;
            else if (ch == '{') ++depth;
            else if (ch == '}') {
                --depth;
                if (depth == 0) return true;
            }
        }
    }
    return false;
}

static bool parseJsonCredentials(const std::string& raw) {
    size_t first = raw.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || raw[first] != '{') return false;

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, raw.substr(first));
    if (error) {
        Serial.printf("[BLE] Error parseando trama JSON: %s\n", error.c_str());
        return false;
    }

    if (doc.containsKey("apn")) {
        String nextApn = doc["apn"].as<String>();
        nextApn.trim();
        if (nextApn.length() > 0) config.apn = nextApn;
    }
    if (doc.containsKey("apn_user")) config.apn_user = doc["apn_user"].as<String>();
    if (doc.containsKey("apn_pass")) config.apn_pass = doc["apn_pass"].as<String>();

    String nextSsid;
    if (doc.containsKey("wifi_ssid")) nextSsid = doc["wifi_ssid"].as<String>();
    else if (doc.containsKey("ssid")) nextSsid = doc["ssid"].as<String>();
    nextSsid.trim();
    if (nextSsid.length() > 0) {
        config.wifi_ssid = nextSsid;
        if (doc.containsKey("wifi_pass")) config.wifi_pass = doc["wifi_pass"].as<String>();
        else if (doc.containsKey("pass")) config.wifi_pass = doc["pass"].as<String>();
        else if (doc.containsKey("password")) config.wifi_pass = doc["password"].as<String>();
    }

    bool hasBrokerBundle = false;
    if (doc.containsKey("mqtt_broker") && doc["mqtt_broker"].as<String>().length() > 0) {
        config.mqtt_broker = doc["mqtt_broker"].as<String>();
        hasBrokerBundle = true;
    } else if (doc.containsKey("broker") && doc["broker"].as<String>().length() > 0) {
        config.mqtt_broker = doc["broker"].as<String>();
        hasBrokerBundle = true;
    } else if (doc.containsKey("server") && doc["server"].as<String>().length() > 0) {
        config.mqtt_broker = doc["server"].as<String>();
        hasBrokerBundle = true;
    } else if (doc.containsKey("serverName") && doc["serverName"].as<String>().length() > 0) {
        config.mqtt_broker = doc["serverName"].as<String>();
        hasBrokerBundle = true;
    }

    if (doc.containsKey("mqtt_port")) config.mqtt_port = doc["mqtt_port"].as<int>();
    else if (doc.containsKey("port")) config.mqtt_port = doc["port"].as<int>();

    if (doc.containsKey("account")) { config.mqtt_user = doc["account"].as<String>(); hasBrokerBundle = true; }
    else if (doc.containsKey("user")) { config.mqtt_user = doc["user"].as<String>(); hasBrokerBundle = true; }
    else if (doc.containsKey("mqtt_user")) { config.mqtt_user = doc["mqtt_user"].as<String>(); hasBrokerBundle = true; }
    else if (doc.containsKey("api_key")) { config.mqtt_user = doc["api_key"].as<String>(); hasBrokerBundle = true; }

    if (doc.containsKey("mqtt_pass")) config.mqtt_pass = doc["mqtt_pass"].as<String>();
    else if (doc.containsKey("pass_mqtt")) config.mqtt_pass = doc["pass_mqtt"].as<String>();

    if (doc.containsKey("device_id") && doc["device_id"].as<String>().length() > 0) config.device_id = doc["device_id"].as<String>();
    else if (doc.containsKey("id") && doc["id"].as<String>().length() > 0) config.device_id = doc["id"].as<String>();
    else if (doc.containsKey("deviceid") && doc["deviceid"].as<String>().length() > 0) config.device_id = doc["deviceid"].as<String>();

    if (doc.containsKey("device_key") && doc["device_key"].as<String>().length() >= 8) config.device_key = doc["device_key"].as<String>();
    else if (doc.containsKey("devicekey") && doc["devicekey"].as<String>().length() >= 8) config.device_key = doc["devicekey"].as<String>();
    else if (doc.containsKey("apikey") && doc["apikey"].as<String>().length() >= 8) {
        if (config.mqtt_user.length() == 0) config.mqtt_user = doc["apikey"].as<String>();
        hasBrokerBundle = true;
    }

    if (doc.containsKey("bind_token")) { config.bind_token = doc["bind_token"].as<String>(); hasBrokerBundle = true; }
    else if (doc.containsKey("token")) { config.bind_token = doc["token"].as<String>(); hasBrokerBundle = true; }

    if (doc.containsKey("ota_hmac_key")) config.ota_hmac_key = doc["ota_hmac_key"].as<String>();
    else if (doc.containsKey("ota_key")) config.ota_hmac_key = doc["ota_key"].as<String>();

    if (doc.containsKey("tank_radius_cm")) config.tank_radius_cm = doc["tank_radius_cm"].as<float>();
    if (doc.containsKey("tank_length_cm")) config.tank_length_cm = doc["tank_length_cm"].as<float>();
    if (doc.containsKey("tank_height_cm")) config.tank_height_cm = doc["tank_height_cm"].as<float>();

    if (doc.containsKey("sledOnline")) {
        String sl = doc["sledOnline"].as<String>();
        sl.toLowerCase();
        config.sled_online = (sl != "off" && sl != "0" && sl != "false");
    }

    if (hasBrokerBundle) {
        config.account_bound = true;
    }

    Serial.println("[BLE] Trama JSON de aprovisionamiento GLP procesada con éxito");
    return true;
}

static bool parseKeyValueFrame(const std::string& raw) {
    auto trim = [](const std::string& s) -> std::string {
        size_t start = s.find_first_not_of(" \t\r\n\"'");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n\"'");
        return s.substr(start, end - start + 1);
    };

    bool matched = false;
    bool hasBrokerBundle = false;
    std::string cur = raw;
    for (char& c : cur) {
        if (c == '&' || c == '\n' || c == '\r') c = ';';
    }

    std::stringstream ss(cur);
    std::string token;
    while (std::getline(ss, token, ';')) {
        if (token.empty()) continue;
        size_t sep = token.find('=');
        if (sep == std::string::npos) sep = token.find(':');
        if (sep == std::string::npos) continue;

        std::string k = trim(token.substr(0, sep));
        std::string v = trim(token.substr(sep + 1));
        for (char& ch : k) ch = tolower((unsigned char)ch);

        if (k == "apn" && !v.empty()) { config.apn = v.c_str(); matched = true; }
        else if (k == "apn_user") { config.apn_user = v.c_str(); matched = true; }
        else if (k == "apn_pass") { config.apn_pass = v.c_str(); matched = true; }
        else if ((k == "wifi_ssid" || k == "ssid") && !v.empty()) { config.wifi_ssid = v.c_str(); matched = true; }
        else if ((k == "wifi_pass" || k == "pass" || k == "password") && !v.empty()) { config.wifi_pass = v.c_str(); matched = true; }
        else if ((k == "mqtt_broker" || k == "broker" || k == "server") && !v.empty()) { config.mqtt_broker = v.c_str(); matched = true; hasBrokerBundle = true; }
        else if (k == "mqtt_port" || k == "port") { config.mqtt_port = atoi(v.c_str()); matched = true; }
        else if ((k == "account" || k == "user" || k == "mqtt_user" || k == "api_key") && !v.empty()) { config.mqtt_user = v.c_str(); matched = true; hasBrokerBundle = true; }
        else if (k == "mqtt_pass" || k == "pass_mqtt") { config.mqtt_pass = v.c_str(); matched = true; }
        else if ((k == "device_id" || k == "id" || k == "deviceid") && !v.empty()) { config.device_id = v.c_str(); matched = true; }
        else if ((k == "device_key" || k == "devicekey" || k == "apikey") && v.size() >= 8) { config.device_key = v.c_str(); matched = true; }
        else if ((k == "bind_token" || k == "token") && !v.empty()) { config.bind_token = v.c_str(); matched = true; hasBrokerBundle = true; }
        else if (k == "ota_hmac_key" || k == "ota_key") { config.ota_hmac_key = v.c_str(); matched = true; }
        else if (k == "tank_radius_cm" || k == "radius") { config.tank_radius_cm = atof(v.c_str()); matched = true; }
        else if (k == "tank_length_cm" || k == "length") { config.tank_length_cm = atof(v.c_str()); matched = true; }
    }

    if (hasBrokerBundle) {
        config.account_bound = true;
    }
    if (matched) {
        Serial.println("[BLE] Trama Clave=Valor de aprovisionamiento GLP procesada con éxito");
    }
    return matched;
}

static bool parseFrame(const std::string& raw) {
    if (raw.empty()) return false;
    if (parseJsonCredentials(raw)) return true;
    if (parseKeyValueFrame(raw)) return true;
    return false;
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        (void)pServer;
        Serial.println("[BLE] App móvil conectada al dispositivo GLP.");
    }
    void onDisconnect(NimBLEServer* pServer) override {
        Serial.println("[BLE] App móvil desconectada.");
        if (!_bleStopping && !_configSaved && pServer != nullptr) {
            NimBLEDevice::startAdvertising();
        }
    }
};

class StringWriteCallback : public NimBLECharacteristicCallbacks {
public:
    String* _target;
    bool _isSecret;
    StringWriteCallback(String* target, bool isSecret) : _target(target), _isSecret(isSecret) {}
    void onRead(NimBLECharacteristic* c) override {
        c->setValue(_isSecret ? "" : _target->c_str());
    }
    void onWrite(NimBLECharacteristic* c) override {
        std::string val = c->getValue();
        if (!_isSecret && val.empty() && _target == &config.wifi_ssid) {
            return;
        }
        *_target = val.c_str();
        if (!_isSecret) {
            c->setValue(val);
            Serial.printf("[BLE] %s = %s\n", c->getUUID().toString().c_str(), _target->c_str());
        } else {
            c->setValue("");
            Serial.printf("[BLE] %s = <redacted>\n", c->getUUID().toString().c_str());
        }
    }
};

class PortWriteCallback : public NimBLECharacteristicCallbacks {
public:
    int* _target;
    explicit PortWriteCallback(int* target) : _target(target) {}
    void onRead(NimBLECharacteristic* c) override {
        c->setValue(String(*_target).c_str());
    }
    void onWrite(NimBLECharacteristic* c) override {
        std::string val = c->getValue();
        if (val.size() == 2 && (!isdigit((unsigned char)val[0]) || !isdigit((unsigned char)val[1]))) {
            uint16_t p = (uint8_t)val[0] | ((uint8_t)val[1] << 8);
            *_target = p;
        } else if (!val.empty()) {
            *_target = atoi(val.c_str());
        }
        c->setValue(String(*_target).c_str());
        Serial.printf("[BLE] Puerto MQTT = %d\n", *_target);
    }
};

class FloatWriteCallback : public NimBLECharacteristicCallbacks {
public:
    float* _target;
    explicit FloatWriteCallback(float* target) : _target(target) {}
    void onRead(NimBLECharacteristic* c) override {
        c->setValue(String(*_target, 2).c_str());
    }
    void onWrite(NimBLECharacteristic* c) override {
        std::string val = c->getValue();
        if (!val.empty()) {
            *_target = static_cast<float>(atof(val.c_str()));
            c->setValue(String(*_target, 2).c_str());
            Serial.printf("[BLE] %s = %.2f\n", c->getUUID().toString().c_str(), *_target);
        }
    }
};

class DevKeyCallback : public NimBLECharacteristicCallbacks {
public:
    void onRead(NimBLECharacteristic* c) override {
        c->setValue(config.device_key.c_str());
    }
};

class BindTokenCallback : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic* c) override {
        std::string val = c->getValue();
        if (!val.empty()) {
            config.bind_token = val.c_str();
            config.account_bound = true;
        }
        c->setValue("");
        Serial.println("[BLE] Bind Token recibido (<redacted>)");
    }
};

class SaveCallback : public NimBLECharacteristicCallbacks {
public:
    void onRead(NimBLECharacteristic* c) override {
        c->setValue(_configSaved ? "OK" : "READY");
    }
    void onWrite(NimBLECharacteristic* c) override {
        std::string val = c->getValue();
        if (!val.empty() && val != "1" && val != "SAVE") {
            parseFrame(val);
        }
        if (config.mqtt_broker.length() > 0 || config.mqtt_user.length() > 0 || config.bind_token.length() > 0) {
            config.account_bound = true;
        }
        saveConfig();
        _configSaved = true;
        Serial.println("[BLE] Configuración GLP guardada en NVS. Enviando ACK 'OK'...");
        c->setValue("OK");
        c->notify();
    }
};

class FrameCallback : public NimBLECharacteristicCallbacks {
public:
    void onRead(NimBLECharacteristic* c) override {
        String json;
        buildBleStateJson(json);
        c->setValue(json.c_str());
    }

    void onWrite(NimBLECharacteristic* c) override {
        std::string val = c->getValue();
        if (val.empty()) return;

        unsigned long now = millis();
        if (!_rxFrameBuffer.empty() && (now - _lastRxChunkMs > 4000 || _rxFrameBuffer.size() + val.size() > 2048)) {
            _rxFrameBuffer.clear();
        }
        _lastRxChunkMs = now;

        size_t firstNonWs = val.find_first_not_of(" \t\r\n");
        bool startsJson = (firstNonWs != std::string::npos && val[firstNonWs] == '{');

        if (_rxFrameBuffer.empty() && startsJson && !isCompleteJsonObject(val)) {
            _rxFrameBuffer = val;
            return;
        }

        if (!_rxFrameBuffer.empty()) {
            _rxFrameBuffer += val;
            if (!isCompleteJsonObject(_rxFrameBuffer)) {
                return;
            }
            val = _rxFrameBuffer;
            _rxFrameBuffer.clear();
        }

        if (parseFrame(val)) {
            saveConfig();
            _configSaved = true;
            c->setValue("OK");
            c->notify();
        } else {
            c->setValue("ERROR:INVALID_FRAME");
            c->notify();
        }
    }
};

void initBLEProvisioning() {}

QuickPairingResult runQuickPairing(unsigned long timeoutMs,
                                   glp::domain::IIndicator* indicator,
                                   glp::domain::IButtonHandler* button) {
    Serial.println("\n[PAIRING] =========================================================");
    Serial.println("[PAIRING] Iniciando QUICK PAIRING GLP (BLE GATT + SmartConfig)");
    Serial.println("[PAIRING] - Pulsación larga (5s): Cambiar a SoftAP GLP-<ID>");
    Serial.println("[PAIRING] - Pulsación corta:      Salir de modo emparejamiento");
    Serial.println("[PAIRING] =========================================================");
    _configSaved = false;
    _bleStopping = false;
    _rxFrameBuffer.clear();

    if (indicator) {
        indicator->show(glp::domain::IndicatorStatus::QuickPairing);
    }

    initConfig();

    const bool alreadyWifiConn = (WiFi.status() == WL_CONNECTED);
    if (!alreadyWifiConn) {
        WiFi.mode(WIFI_STA);
        WiFi.beginSmartConfig();
    }

    String advName = "GLP-" + config.device_id;
    NimBLEDevice::init(advName.c_str());
    NimBLEDevice::setMTU(512);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->advertiseOnDisconnect(false);
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* dis = server->createService(NimBLEUUID((uint16_t)0x180A));
    if (dis) {
        NimBLECharacteristic* mChar = dis->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A24), NIMBLE_PROPERTY::READ);
        mChar->setValue("VESIS-GLP-S3");

        NimBLECharacteristic* sChar = dis->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A25), NIMBLE_PROPERTY::READ);
        sChar->setValue(config.device_id.c_str());

        NimBLECharacteristic* fwChar = dis->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A26), NIMBLE_PROPERTY::READ);
        fwChar->setValue(GLP_FIRMWARE_VERSION);

        NimBLECharacteristic* mfChar = dis->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A29), NIMBLE_PROPERTY::READ);
        mfChar->setValue("VESIS");

        dis->start();
    }

    NimBLEService* service = server->createService(SERVICE_UUID);

    auto mkChar = [&](const char* uuid, String* field, bool isSecret) {
        uint32_t props = isSecret ? NIMBLE_PROPERTY::WRITE : (NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
        NimBLECharacteristic* ch = service->createCharacteristic(uuid, props);
        ch->setCallbacks(new StringWriteCallback(field, isSecret));
        if (!isSecret) {
            ch->setValue(field->c_str());
        }
        return ch;
    };

    mkChar(CHAR_APN_UUID,      &config.apn,          false);
    mkChar(CHAR_APNUSER_UUID,  &config.apn_user,     false);
    mkChar(CHAR_APNPASS_UUID,  &config.apn_pass,     true);
    mkChar(CHAR_BROKER_UUID,   &config.mqtt_broker,  false);
    mkChar(CHAR_MQTTUSER_UUID, &config.mqtt_user,    true);
    mkChar(CHAR_MQTTPASS_UUID, &config.mqtt_pass,    true);
    mkChar(CHAR_DEVID_UUID,    &config.device_id,    false);
    mkChar(CHAR_OTAKEY_UUID,   &config.ota_hmac_key, true);
    mkChar(CHAR_WIFISSID_UUID, &config.wifi_ssid,    false);
    mkChar(CHAR_WIFIPASS_UUID, &config.wifi_pass,    true);

    NimBLECharacteristic* portChar = service->createCharacteristic(
        CHAR_PORT_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    portChar->setCallbacks(new PortWriteCallback(&config.mqtt_port));
    portChar->setValue(String(config.mqtt_port).c_str());

    NimBLECharacteristic* radiusChar = service->createCharacteristic(
        CHAR_RADIUS_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    radiusChar->setCallbacks(new FloatWriteCallback(&config.tank_radius_cm));
    radiusChar->setValue(String(config.tank_radius_cm, 2).c_str());

    NimBLECharacteristic* lengthChar = service->createCharacteristic(
        CHAR_LENGTH_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    lengthChar->setCallbacks(new FloatWriteCallback(&config.tank_length_cm));
    lengthChar->setValue(String(config.tank_length_cm, 2).c_str());

    NimBLECharacteristic* devKeyChar = service->createCharacteristic(
        CHAR_DEVKEY_UUID, NIMBLE_PROPERTY::READ);
    devKeyChar->setCallbacks(new DevKeyCallback());
    devKeyChar->setValue(config.device_key.c_str());

    NimBLECharacteristic* bindTokenChar = service->createCharacteristic(
        CHAR_BINDTOKEN_UUID, NIMBLE_PROPERTY::WRITE);
    bindTokenChar->setCallbacks(new BindTokenCallback());
    bindTokenChar->setValue("");

    NimBLECharacteristic* frameChar = service->createCharacteristic(
        CHAR_FRAME_JSON_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    frameChar->setCallbacks(new FrameCallback());
    String initJson;
    buildBleStateJson(initJson);
    frameChar->setValue(initJson.c_str());

    NimBLECharacteristic* saveChar = service->createCharacteristic(
        CHAR_SAVE_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    saveChar->setCallbacks(new SaveCallback());
    saveChar->setValue("READY");

    service->start();

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setName(advName.c_str());

    const uint8_t advState = computeAdvStateByte();
    const uint8_t advFlags = computeAdvProvFlags();
    std::string mfgData;
    mfgData.push_back((char)0xFF);
    mfgData.push_back((char)0xFF);
    mfgData.push_back('V');
    mfgData.push_back('S');
    mfgData.push_back((char)advState);
    mfgData.push_back((char)0x04); // deviceCode: 0x04 = glp_pressure
    mfgData.push_back((char)advFlags);
    adv->setManufacturerData(mfgData);
    adv->start();

    Serial.printf("[BLE] Publicitando como '%s' (stateByte=0x%02X, devCode=0x04, flags=0x%02X, mode=%s)\n",
                  advName.c_str(),
                  (unsigned)advState,
                  (unsigned)advFlags,
                  resolveDeviceCredentialMode(alreadyWifiConn));

    if (button) {
        unsigned long releaseWaitStart = millis();
        while (digitalRead(button->getPin()) == LOW && (millis() - releaseWaitStart) < 3000) {
            if (indicator) indicator->update();
            delay(20);
        }
        button->init();
    }

    auto cleanupRadios = [&]() {
        _bleStopping = true;
        if (!alreadyWifiConn) {
            WiFi.stopSmartConfig();
        }
        server->advertiseOnDisconnect(false);
        adv->stop();
        auto peers = server->getPeerDevices();
        for (uint16_t connId : peers) {
            server->disconnect(connId);
        }
        unsigned long discWait = millis();
        while (server->getConnectedCount() > 0 && (millis() - discWait) < 600) {
            delay(20);
        }
        delay(80);
        NimBLEDevice::deinit(true);
    };

    unsigned long start = millis();
    while ((millis() - start) < timeoutMs) {
        if (indicator) {
            indicator->update();
        }

        if (_configSaved) {
            Serial.println("[PAIRING] ¡Aprovisionamiento BLE GLP completado con éxito!");
            delay(500);
            cleanupRadios();
            return QuickPairingResult::Provisioned;
        }

        if (!alreadyWifiConn && WiFi.smartConfigDone()) {
            String scSsid = WiFi.SSID();
            String scPass = WiFi.psk();
            if (scSsid.length() > 0) {
                config.wifi_ssid = scSsid;
                config.wifi_pass = scPass;
                saveConfig();
                Serial.printf("[PAIRING] ¡SmartConfig GLP completado! SSID: %s\n", scSsid.c_str());
                cleanupRadios();
                return QuickPairingResult::Provisioned;
            }
        }

        if (button) {
            auto ev = button->update();
            if (ev == glp::domain::ButtonEvent::LongPress) {
                Serial.println("[PAIRING] 2ª pulsación larga (5s) -> Conmutando a SoftAP GLP-CONFIG-<ID>...");
                cleanupRadios();
                return QuickPairingResult::SwitchToSoftAp;
            } else if (ev == glp::domain::ButtonEvent::ShortPress) {
                Serial.println("[PAIRING] Pulsación corta detectada -> Cancelando modo emparejamiento.");
                cleanupRadios();
                return QuickPairingResult::CancelledByButton;
            }
        }

        delay(20);
    }

    Serial.println("[PAIRING] Tiempo de espera de Quick Pairing agotado.");
    cleanupRadios();
    return QuickPairingResult::Timeout;
}

bool runBLEProvisioning(unsigned long timeoutMs) {
    return runQuickPairing(timeoutMs, nullptr, nullptr) == QuickPairingResult::Provisioned;
}
