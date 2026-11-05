#include "Infrastructure/Provisioning/wifi_ap_provisioning.h"
#include "Infrastructure/Config/nvs_config.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <cstring>

static AsyncWebServer webServer(80);
static volatile bool _apConfigSaved = false;

static String buildHtmlForm() {
    String html;
    html.reserve(2800);
    html += F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
              "<meta name='viewport' content='width=device-width, initial-scale=1'>"
              "<title>GLP Sensor Config</title>"
              "<style>"
              "body{font-family:Arial,sans-serif;max-width:480px;margin:40px auto;padding:0 20px;background:#f5f5f5}"
              "h2{color:#2c3e50}"
              "label{display:block;margin-top:12px;font-weight:bold;color:#34495e;font-size:14px}"
              "input{width:100%;padding:8px;margin-top:4px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}"
              "button{margin-top:20px;width:100%;padding:12px;background:#2980b9;color:#fff;border:none;border-radius:4px;font-size:16px;cursor:pointer}"
              "button:hover{background:#1a6da0}"
              ".section{background:#fff;padding:16px;border-radius:8px;margin-top:16px;box-shadow:0 1px 3px rgba(0,0,0,.1)}"
              "</style></head><body>"
              "<h2>Configuracion Sensor GLP</h2>"
              "<form method='POST' action='/save'>"
              "<div class='section'><h3>SIM / Datos moviles</h3>"
              "<label>APN</label><input name='apn' value='");
    html += config.apn.length() > 0 ? config.apn : String(getCellularResolvedApn());
    html += F("'><label>Usuario APN</label><input name='apn_user' value='");
    html += config.apn_user;
    html += F("'><label>Clave APN</label><input name='apn_pass' type='password' value=''>"
              "</div><div class='section'><h3>WiFi (Opcional)</h3>"
              "<label>SSID WiFi</label><input name='wifi_ssid' value='");
    html += config.wifi_ssid;
    html += F("'><label>Clave WiFi</label><input name='wifi_pass' type='password' value=''>"
              "</div><div class='section'><h3>Servidor MQTT 5.0</h3>"
              "<label>Broker (IP o dominio)</label><input name='mqtt_broker' value='");
    html += config.mqtt_broker;
    html += F("' required><label>Puerto</label><input name='mqtt_port' type='number' value='");
    html += String(config.mqtt_port > 0 ? config.mqtt_port : 8883);
    html += F("'><label>Usuario MQTT</label><input name='mqtt_user' value='");
    html += config.mqtt_user;
    html += F("'><label>Clave MQTT</label><input name='mqtt_pass' type='password' value=''>"
              "<label>Bind Token</label><input name='bind_token' value='");
    html += config.bind_token;
    html += F("'></div><div class='section'><h3>Tanque e Identificacion</h3>"
              "<label>ID del dispositivo</label><input name='device_id' value='");
    html += config.device_id;
    html += F("' required><label>Radio del tanque (cm)</label><input name='tank_r' type='number' step='0.1' value='");
    html += String(config.tank_radius_cm, 1);
    html += F("'><label>Largo del tanque (cm)</label><input name='tank_l' type='number' step='0.1' value='");
    html += String(config.tank_length_cm, 1);
    html += F("'><label>Clave HMAC OTA (hex 64 chars)</label><input name='ota_hmac' value=''></div>"
              "<button type='submit'>Guardar y reiniciar</button></form></body></html>");
    return html;
}

static void sendDeviceMetadataJson(AsyncWebServerRequest* req) {
    const bool wifiConn = false;
    const bool cellConn = isCellularDataConnected();
    const bool autoApn  = isCellularApnAutoDetected();
    const char* credMode = resolveDeviceCredentialMode(wifiConn);
    const bool adoptReady = (std::strcmp(credMode, "adopt_only") == 0);
    const String effectiveApn = config.apn.length() > 0 ? config.apn : String(getCellularResolvedApn());

    StaticJsonDocument<640> doc;
    doc["deviceid"]              = config.device_id;
    doc["device_id"]             = config.device_id;
    doc["chipid"]                = config.device_id;
    doc["apikey"]                = config.device_key;
    doc["device_key"]            = config.device_key;
    doc["device_family"]         = "glp_pressure";
    doc["device_code"]           = 4;
    doc["device_type"]           = "GLP-Sensor";
    doc["model"]                 = "VESIS-GLP-S3";
    doc["channel_count"]         = 1;
    doc["version"]               = GLP_FIRMWARE_VERSION;
    doc["config_state"]          = resolveDeviceConfigState(wifiConn, cellConn, false);
    doc["credential_mode"]       = credMode;
    doc["requires_wifi"]         = (std::strcmp(credMode, "wifi") == 0 || std::strcmp(credMode, "wifi_and_apn") == 0);
    doc["requires_apn"]          = (std::strcmp(credMode, "apn") == 0 || std::strcmp(credMode, "wifi_and_apn") == 0);
    doc["supports_wifi"]         = true;
    doc["supports_apn"]          = true;
    doc["sim_present"]           = isCellularSimPresent();
    doc["apn_auto_detected"]     = autoApn;
    doc["cellular_connected"]    = cellConn;
    doc["adopt_ready"]           = adoptReady;
    doc["apn"]                   = effectiveApn;
    doc["account_bound"]         = isAccountBound();
    doc["has_wifi_credentials"]  = hasValidWifiCredentials();
    doc["has_broker_credentials"]= hasValidBrokerCredentials();
    doc["tank_radius_cm"]        = config.tank_radius_cm;
    doc["tank_length_cm"]        = config.tank_length_cm;
    doc["accept"]                = "post";

    String out;
    serializeJson(doc, out);
    AsyncWebServerResponse* res = req->beginResponse(200, "application/json", out);
    res->addHeader("Connection", "close");
    req->send(res);
}

static void handleProvisioningJsonBody(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index + len != total) return;

    StaticJsonDocument<640> doc;
    if (deserializeJson(doc, data, len) != DeserializationError::Ok) {
        AsyncWebServerResponse* res = req->beginResponse(400, "application/json", "{\"error\":400}");
        res->addHeader("Connection", "close");
        req->send(res);
        return;
    }

    String nextSsid = doc["ssid"] | (doc["wifi_ssid"] | "");
    nextSsid.trim();
    String nextApn  = doc["apn"] | "";
    nextApn.trim();

    const bool hasBrokerBundle =
        doc.containsKey("serverName") || doc.containsKey("mqtt_broker") ||
        doc.containsKey("server") || doc.containsKey("broker") ||
        doc.containsKey("apikey") || doc.containsKey("mqtt_user") ||
        doc.containsKey("account") || doc.containsKey("bind_token") ||
        doc.containsKey("token");

    const bool canAdoptWithoutNewNet =
        isCellularApnAutoDetected() || isCellularDataConnected() ||
        config.apn.length() > 0 || hasValidWifiCredentials();

    if (nextSsid.length() == 0 && nextApn.length() == 0 && !(hasBrokerBundle && canAdoptWithoutNewNet)) {
        AsyncWebServerResponse* res = req->beginResponse(400, "application/json", "{\"error\":400,\"msg\":\"missing_network_or_apn\"}");
        res->addHeader("Connection", "close");
        req->send(res);
        return;
    }

    if (nextSsid.length() > 0) {
        config.wifi_ssid = nextSsid;
        if (doc.containsKey("password")) config.wifi_pass = doc["password"].as<String>();
        else if (doc.containsKey("pass")) config.wifi_pass = doc["pass"].as<String>();
        else if (doc.containsKey("wifi_pass")) config.wifi_pass = doc["wifi_pass"].as<String>();
    }

    if (nextApn.length() > 0) {
        config.apn = nextApn;
    }
    if (doc.containsKey("apn_user")) config.apn_user = doc["apn_user"].as<String>();
    if (doc.containsKey("apn_pass")) config.apn_pass = doc["apn_pass"].as<String>();

    if (doc.containsKey("serverName") && doc["serverName"].as<String>().length() > 0) config.mqtt_broker = doc["serverName"].as<String>();
    else if (doc.containsKey("mqtt_broker") && doc["mqtt_broker"].as<String>().length() > 0) config.mqtt_broker = doc["mqtt_broker"].as<String>();
    else if (doc.containsKey("server") && doc["server"].as<String>().length() > 0) config.mqtt_broker = doc["server"].as<String>();
    else if (doc.containsKey("broker") && doc["broker"].as<String>().length() > 0) config.mqtt_broker = doc["broker"].as<String>();

    if (doc.containsKey("port")) config.mqtt_port = doc["port"].as<int>();
    else if (doc.containsKey("mqtt_port")) config.mqtt_port = doc["mqtt_port"].as<int>();

    if (doc.containsKey("mqtt_user")) config.mqtt_user = doc["mqtt_user"].as<String>();
    else if (doc.containsKey("account")) config.mqtt_user = doc["account"].as<String>();
    else if (doc.containsKey("apikey")) config.mqtt_user = doc["apikey"].as<String>();

    if (doc.containsKey("mqtt_pass")) config.mqtt_pass = doc["mqtt_pass"].as<String>();
    if (doc.containsKey("device_id") && doc["device_id"].as<String>().length() > 0) config.device_id = doc["device_id"].as<String>();
    if (doc.containsKey("devicekey") && doc["devicekey"].as<String>().length() >= 8) config.device_key = doc["devicekey"].as<String>();
    else if (doc.containsKey("device_key") && doc["device_key"].as<String>().length() >= 8) config.device_key = doc["device_key"].as<String>();

    if (doc.containsKey("bind_token")) config.bind_token = doc["bind_token"].as<String>();
    else if (doc.containsKey("token")) config.bind_token = doc["token"].as<String>();

    if (doc.containsKey("tank_radius_cm")) {
        config.tank_radius_cm = doc["tank_radius_cm"].as<float>();
        config.tank_height_cm = 2.0f * config.tank_radius_cm;
    }
    if (doc.containsKey("tank_length_cm")) config.tank_length_cm = doc["tank_length_cm"].as<float>();
    if (doc.containsKey("tank_height_cm")) config.tank_height_cm = doc["tank_height_cm"].as<float>();
    if (doc.containsKey("propane_fraction")) config.propane_fraction = doc["propane_fraction"].as<float>();
    if (doc.containsKey("ota_hmac_key")) config.ota_hmac_key = doc["ota_hmac_key"].as<String>();
    if (doc.containsKey("sledOnline")) {
        String sl = doc["sledOnline"].as<String>();
        sl.toLowerCase();
        config.sled_online = (sl != "off" && sl != "0" && sl != "false");
    }

    if (hasBrokerBundle) {
        config.account_bound = true;
    }

    saveConfig();
    AsyncWebServerResponse* res = req->beginResponse(200, "application/json", "{\"error\":0}");
    res->addHeader("Connection", "close");
    req->send(res);
    _apConfigSaved = true;
}

void initWiFiAPProvisioning() {}

bool runWiFiAPProvisioning(unsigned long timeoutMs,
                           glp::domain::IIndicator* indicator,
                           glp::domain::IButtonHandler* button) {
    _apConfigSaved = false;

    initConfig();

    if (indicator) {
        indicator->show(glp::domain::IndicatorStatus::CompatibleAp);
    }

    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false, false);
    delay(100);

    String apName = "GLP-CONFIG-" + (config.device_id.length() > 0 ? config.device_id : "0001");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), "glp12345", 1, 0, 4);
    Serial.printf("[WiFi AP] Red creada: %s | IP: %s\n",
                  apName.c_str(), WiFi.softAPIP().toString().c_str());

    webServer.reset();

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        AsyncWebServerResponse* res = req->beginResponse(200, "text/html", buildHtmlForm());
        res->addHeader("Connection", "close");
        req->send(res);
    });

    webServer.on("/device", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendDeviceMetadataJson(req);
    });

    webServer.on("/api/info", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendDeviceMetadataJson(req);
    });

    webServer.on(
        "/ap", HTTP_POST,
        [](AsyncWebServerRequest* req) { (void)req; },
        nullptr,
        handleProvisioningJsonBody);

    webServer.on(
        "/api/wifi", HTTP_POST,
        [](AsyncWebServerRequest* req) { (void)req; },
        nullptr,
        handleProvisioningJsonBody);

    webServer.on("/save", HTTP_POST, [](AsyncWebServerRequest* req) {
        auto get = [&](const char* name) -> String {
            return req->hasParam(name, true) ? req->getParam(name, true)->value() : "";
        };
        if (get("apn").length() > 0)          config.apn          = get("apn");
        if (req->hasParam("apn_user", true))  config.apn_user     = get("apn_user");
        if (get("apn_pass").length() > 0)     config.apn_pass     = get("apn_pass");
        if (get("wifi_ssid").length() > 0)    config.wifi_ssid    = get("wifi_ssid");
        if (get("wifi_pass").length() > 0)    config.wifi_pass    = get("wifi_pass");
        if (get("mqtt_broker").length() > 0)  { config.mqtt_broker = get("mqtt_broker"); config.account_bound = true; }
        if (get("mqtt_port").toInt() > 0)     config.mqtt_port    = get("mqtt_port").toInt();
        if (get("mqtt_user").length() > 0)    config.mqtt_user    = get("mqtt_user");
        if (get("mqtt_pass").length() > 0)    config.mqtt_pass    = get("mqtt_pass");
        if (get("bind_token").length() > 0)   { config.bind_token = get("bind_token"); config.account_bound = true; }
        if (get("device_id").length() > 0)    config.device_id    = get("device_id");
        if (get("tank_r").toFloat() > 0.0f) {
            config.tank_radius_cm = get("tank_r").toFloat();
            config.tank_height_cm = 2.0f * config.tank_radius_cm;
        }
        if (get("tank_l").toFloat() > 0.0f)   config.tank_length_cm = get("tank_l").toFloat();
        if (get("ota_hmac").length() > 0)     config.ota_hmac_key   = get("ota_hmac");
        saveConfig();

        AsyncWebServerResponse* res = req->beginResponse(
            200, "text/html",
            "<html><body style='font-family:Arial;text-align:center;margin-top:60px'>"
            "<h2>Configuracion guardada</h2><p>El dispositivo se reiniciara en 3 segundos...</p>"
            "</body></html>");
        res->addHeader("Connection", "close");
        req->send(res);
        _apConfigSaved = true;
    });

    if (button) {
        unsigned long releaseStart = millis();
        while (digitalRead(button->getPin()) == LOW && (millis() - releaseStart) < 3000) {
            if (indicator) indicator->update();
            delay(20);
        }
        button->init();
    }

    webServer.begin();

    unsigned long start = millis();
    while (!_apConfigSaved && (millis() - start < timeoutMs)) {
        if (indicator) {
            indicator->update();
        }
        if (button) {
            auto ev = button->update();
            if (ev == glp::domain::ButtonEvent::ShortPress) {
                Serial.println("[WiFi AP] Pulsación corta detectada -> Saliendo de portal SoftAP GLP.");
                break;
            }
        }
        delay(20);
    }

    delay(400);
    webServer.end();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    return _apConfigSaved;
}
