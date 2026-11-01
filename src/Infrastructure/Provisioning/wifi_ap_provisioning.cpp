#include "Infrastructure/Provisioning/wifi_ap_provisioning.h"
#include "Infrastructure/Config/nvs_config.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer webServer(80);
static bool _apConfigSaved = false;

static const char HTML_FORM[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>GLP Sensor Config</title>
  <style>
    body{font-family:Arial,sans-serif;max-width:480px;margin:40px auto;padding:0 20px;background:#f5f5f5}
    h2{color:#2c3e50}
    label{display:block;margin-top:12px;font-weight:bold;color:#34495e;font-size:14px}
    input{width:100%;padding:8px;margin-top:4px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}
    button{margin-top:20px;width:100%;padding:12px;background:#2980b9;color:#fff;border:none;border-radius:4px;font-size:16px;cursor:pointer}
    button:hover{background:#1a6da0}
    .section{background:#fff;padding:16px;border-radius:8px;margin-top:16px;box-shadow:0 1px 3px rgba(0,0,0,.1)}
  </style>
</head>
<body>
  <h2>⚙️ Configuración Sensor GLP</h2>
  <form method="POST" action="/save">
    <div class="section">
      <h3>📡 SIM / Datos móviles</h3>
      <label>APN</label><input name="apn" value="internet.claro.com.do">
      <label>Usuario APN</label><input name="apn_user">
      <label>Contraseña APN</label><input name="apn_pass" type="password">
    </div>
    <div class="section">
      <h3>📶 WiFi</h3>
      <label>SSID WiFi</label><input name="wifi_ssid">
      <label>Contraseña WiFi</label><input name="wifi_pass" type="password">
    </div>
    <div class="section">
      <h3>☁️ Servidor MQTT 5.0</h3>
      <label>Broker (IP o dominio)</label><input name="mqtt_broker" required>
      <label>Puerto</label><input name="mqtt_port" type="number" value="8883">
      <label>Usuario MQTT</label><input name="mqtt_user">
      <label>Contraseña MQTT</label><input name="mqtt_pass" type="password">
      <label>Bind Token</label><input name="bind_token">
    </div>
    <div class="section">
      <h3>🛢️ Tanque e Identificación</h3>
      <label>ID del dispositivo</label><input name="device_id" required>
      <label>Radio del tanque (cm)</label><input name="tank_r" type="number" step="0.1" value="50">
      <label>Largo del tanque (cm)</label><input name="tank_l" type="number" step="0.1" value="200">
      <label>Clave HMAC OTA (hex 64 chars)</label><input name="ota_hmac">
    </div>
    <button type="submit">💾 Guardar y reiniciar</button>
  </form>
</body>
</html>
)rawliteral";

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
        req->send_P(200, "text/html", HTML_FORM);
    });

    webServer.on("/save", HTTP_POST, [](AsyncWebServerRequest* req) {
        auto get = [&](const char* name) -> String {
            return req->hasParam(name, true) ? req->getParam(name, true)->value() : "";
        };
        config.apn            = get("apn");
        config.apn_user       = get("apn_user");
        config.apn_pass       = get("apn_pass");
        config.wifi_ssid      = get("wifi_ssid");
        config.wifi_pass      = get("wifi_pass");
        config.mqtt_broker    = get("mqtt_broker");
        config.mqtt_port      = get("mqtt_port").toInt();
        config.mqtt_user      = get("mqtt_user");
        config.mqtt_pass      = get("mqtt_pass");
        if (get("bind_token").length() > 0) config.bind_token = get("bind_token");
        if (get("device_id").length() > 0) config.device_id = get("device_id");
        config.tank_radius_cm = get("tank_r").toFloat();
        config.tank_length_cm = get("tank_l").toFloat();
        config.ota_hmac_key   = get("ota_hmac");
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
