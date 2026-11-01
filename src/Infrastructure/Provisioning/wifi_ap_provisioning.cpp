#include "Infrastructure/Provisioning/wifi_ap_provisioning.h"
#include "Infrastructure/Config/nvs_config.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer webServer(80);
static bool _apConfigSaved = false;

static String buildHtmlForm() {
    String html;
    html.reserve(2600);
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
              "<h2>⚙️ Configuración Sensor GLP</h2>"
              "<form method='POST' action='/save'>"
              "<div class='section'><h3>📡 SIM / Datos móviles</h3>"
              "<label>APN</label><input name='apn' value='");
    html += config.apn;
    html += F("'><label>Usuario APN</label><input name='apn_user' value='");
    html += config.apn_user;
    html += F("'><label>Contraseña APN</label><input name='apn_pass' type='password' value='");
    html += config.apn_pass;
    html += F("'></div><div class='section'><h3>📶 WiFi</h3>"
              "<label>SSID WiFi</label><input name='wifi_ssid' value='");
    html += config.wifi_ssid;
    html += F("'><label>Contraseña WiFi</label><input name='wifi_pass' type='password' value='");
    html += config.wifi_pass;
    html += F("'></div><div class='section'><h3>☁️ Servidor MQTT 5.0</h3>"
              "<label>Broker (IP o dominio)</label><input name='mqtt_broker' value='");
    html += config.mqtt_broker;
    html += F("' required><label>Puerto</label><input name='mqtt_port' type='number' value='");
    html += String(config.mqtt_port > 0 ? config.mqtt_port : 8883);
    html += F("'><label>Usuario MQTT</label><input name='mqtt_user' value='");
    html += config.mqtt_user;
    html += F("'><label>Contraseña MQTT</label><input name='mqtt_pass' type='password' value='");
    html += config.mqtt_pass;
    html += F("'><label>Bind Token</label><input name='bind_token' value='");
    html += config.bind_token;
    html += F("'></div><div class='section'><h3>🛢️ Tanque e Identificación</h3>"
              "<label>ID del dispositivo</label><input name='device_id' value='");
    html += config.device_id;
    html += F("' required><label>Radio del tanque (cm)</label><input name='tank_r' type='number' step='0.1' value='");
    html += String(config.tank_radius_cm, 1);
    html += F("'><label>Largo del tanque (cm)</label><input name='tank_l' type='number' step='0.1' value='");
    html += String(config.tank_length_cm, 1);
    html += F("'><label>Clave HMAC OTA (hex 64 chars)</label><input name='ota_hmac' value='");
    html += config.ota_hmac_key;
    html += F("'></div><button type='submit'>💾 Guardar y reiniciar</button></form></body></html>");
    return html;
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

    String apName = "GLP-CONFIG-" + (config.device_id.length() > 0 ? config.device_id : "0001");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), "glp12345");
    Serial.printf("[WiFi AP] Red creada: %s | IP: %s\n",
                  apName.c_str(), WiFi.softAPIP().toString().c_str());

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", buildHtmlForm());
    });

    // Endpoint Compatible Pairing Mode: GET /device
    webServer.on("/device", HTTP_GET, [](AsyncWebServerRequest* req) {
        StaticJsonDocument<320> doc;
        doc["deviceid"]       = config.device_id;
        doc["apikey"]         = config.device_key;
        doc["chipid"]         = config.device_id;
        doc["device_type"]    = "GLP-Sensor";
        doc["apn"]            = config.apn;
        doc["tank_radius_cm"] = config.tank_radius_cm;
        doc["tank_length_cm"] = config.tank_length_cm;
        doc["accept"]         = "post";
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // Endpoint Compatible Pairing Mode: POST /ap (JSON body)
    webServer.on(
        "/ap", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index + len != total) return;
            StaticJsonDocument<512> doc;
            if (deserializeJson(doc, data, len) != DeserializationError::Ok ||
                (!doc.containsKey("ssid") && !doc.containsKey("wifi_ssid") && !doc.containsKey("apn"))) {
                req->send(400, "application/json", "{\"error\":400}");
                return;
            }

            if (doc.containsKey("ssid")) config.wifi_ssid = doc["ssid"].as<String>();
            else if (doc.containsKey("wifi_ssid")) config.wifi_ssid = doc["wifi_ssid"].as<String>();

            if (doc.containsKey("password")) config.wifi_pass = doc["password"].as<String>();
            else if (doc.containsKey("pass")) config.wifi_pass = doc["pass"].as<String>();
            else if (doc.containsKey("wifi_pass")) config.wifi_pass = doc["wifi_pass"].as<String>();

            if (doc.containsKey("apn")) config.apn = doc["apn"].as<String>();
            if (doc.containsKey("apn_user")) config.apn_user = doc["apn_user"].as<String>();
            if (doc.containsKey("apn_pass")) config.apn_pass = doc["apn_pass"].as<String>();

            if (doc.containsKey("serverName")) config.mqtt_broker = doc["serverName"].as<String>();
            else if (doc.containsKey("mqtt_broker")) config.mqtt_broker = doc["mqtt_broker"].as<String>();
            else if (doc.containsKey("server")) config.mqtt_broker = doc["server"].as<String>();
            else if (doc.containsKey("broker")) config.mqtt_broker = doc["broker"].as<String>();

            if (doc.containsKey("port")) config.mqtt_port = doc["port"].as<int>();
            else if (doc.containsKey("mqtt_port")) config.mqtt_port = doc["mqtt_port"].as<int>();

            if (doc.containsKey("apikey")) config.mqtt_user = doc["apikey"].as<String>();
            else if (doc.containsKey("mqtt_user")) config.mqtt_user = doc["mqtt_user"].as<String>();
            else if (doc.containsKey("account")) config.mqtt_user = doc["account"].as<String>();

            if (doc.containsKey("mqtt_pass")) config.mqtt_pass = doc["mqtt_pass"].as<String>();
            if (doc.containsKey("device_id")) config.device_id = doc["device_id"].as<String>();
            if (doc.containsKey("devicekey")) config.device_key = doc["devicekey"].as<String>();
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

            saveConfig();
            req->send(200, "application/json", "{\"error\":0}");
            _apConfigSaved = true;
        });

    webServer.on("/save", HTTP_POST, [](AsyncWebServerRequest* req) {
        auto get = [&](const char* name) -> String {
            return req->hasParam(name, true) ? req->getParam(name, true)->value() : "";
        };
        if (get("apn").length() > 0)         config.apn         = get("apn");
        if (req->hasParam("apn_user", true)) config.apn_user    = get("apn_user");
        if (req->hasParam("apn_pass", true)) config.apn_pass    = get("apn_pass");
        if (get("wifi_ssid").length() > 0)   config.wifi_ssid   = get("wifi_ssid");
        if (req->hasParam("wifi_pass", true))config.wifi_pass   = get("wifi_pass");
        if (get("mqtt_broker").length() > 0) config.mqtt_broker = get("mqtt_broker");
        if (get("mqtt_port").toInt() > 0)    config.mqtt_port   = get("mqtt_port").toInt();
        if (get("mqtt_user").length() > 0)   config.mqtt_user   = get("mqtt_user");
        if (req->hasParam("mqtt_pass", true))config.mqtt_pass   = get("mqtt_pass");
        if (get("bind_token").length() > 0)  config.bind_token  = get("bind_token");
        if (get("device_id").length() > 0)   config.device_id   = get("device_id");
        if (get("tank_r").toFloat() > 0.0f) {
            config.tank_radius_cm = get("tank_r").toFloat();
            config.tank_height_cm = 2.0f * config.tank_radius_cm;
        }
        if (get("tank_l").toFloat() > 0.0f)  config.tank_length_cm = get("tank_l").toFloat();
        if (get("ota_hmac").length() > 0)    config.ota_hmac_key   = get("ota_hmac");
        saveConfig();

        req->send(200, "text/html",
            "<html><body style='font-family:Arial;text-align:center;margin-top:60px'>"
            "<h2>✅ Configuración guardada</h2><p>El dispositivo se reiniciará en 3 segundos...</p>"
            "</body></html>");
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
    return _apConfigSaved;
}
