#include "wifi_ap_provisioning.h"
#include "Infrastructure/Config/nvs_config.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer _server(80);
static DNSServer      _dns;
static bool           _configSaved = false;

// Minimal HTML form with inline CSS — served from PROGMEM to save RAM
static const char CONFIG_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>GLP Config Portal</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;display:flex;justify-content:center;align-items:center;padding:16px}
    .card{background:#1e293b;border-radius:16px;padding:32px;width:100%;max-width:480px;box-shadow:0 25px 50px rgba(0,0,0,.5)}
    h1{font-size:1.5rem;font-weight:700;margin-bottom:8px;color:#38bdf8}
    p.sub{font-size:.85rem;color:#94a3b8;margin-bottom:24px}
    .section{margin-bottom:20px}
    .section-title{font-size:.7rem;font-weight:600;text-transform:uppercase;letter-spacing:.1em;color:#64748b;margin-bottom:10px}
    label{display:block;font-size:.85rem;color:#cbd5e1;margin-bottom:4px}
    input{width:100%;padding:10px 14px;background:#0f172a;border:1px solid #334155;border-radius:8px;color:#f1f5f9;font-size:.95rem;transition:border .2s}
    input:focus{outline:none;border-color:#38bdf8}
    button{width:100%;padding:14px;background:linear-gradient(135deg,#0ea5e9,#6366f1);color:#fff;font-size:1rem;font-weight:600;border:none;border-radius:10px;cursor:pointer;margin-top:24px;transition:opacity .2s}
    button:hover{opacity:.88}
    .gap{height:10px}
  </style>
</head>
<body>
<div class="card">
  <h1>⚙️ GLP Config Portal</h1>
  <p class="sub">Configura el dispositivo y presiona Guardar para reiniciar.</p>
  <form action="/save" method="POST">
    <div class="section">
      <div class="section-title">Celular / APN</div>
      <label>APN</label><input name="apn" value="{{apn}}" placeholder="internet.claro.com">
      <div class="gap"></div>
      <label>Usuario APN</label><input name="apn_user" value="{{apn_user}}" placeholder="(vacío si no aplica)">
      <div class="gap"></div>
      <label>Contraseña APN</label><input name="apn_pass" value="{{apn_pass}}" type="password">
    </div>
    <div class="section">
      <div class="section-title">Red WiFi (opcional — uplink preferido)</div>
      <label>SSID</label><input name="wifi_ssid" value="{{wifi_ssid}}" placeholder="(vacío = solo celular)">
      <div class="gap"></div>
      <label>Contraseña WiFi</label><input name="wifi_pass" value="{{wifi_pass}}" type="password">
    </div>
    <div class="section">
      <div class="section-title">Broker MQTT 5.0</div>
      <label>Host</label><input name="mqtt_broker" value="{{mqtt_broker}}" placeholder="broker.servidor.com">
      <div class="gap"></div>
      <label>Puerto</label><input name="mqtt_port" value="{{mqtt_port}}" placeholder="1883">
      <div class="gap"></div>
      <label>Usuario</label><input name="mqtt_user" value="{{mqtt_user}}">
      <div class="gap"></div>
      <label>Contraseña</label><input name="mqtt_pass" value="{{mqtt_pass}}" type="password">
    </div>
    <div class="section">
      <div class="section-title">Dispositivo</div>
      <label>ID del Dispositivo</label><input name="device_id" value="{{device_id}}" placeholder="GLP-001">
    </div>
    <div class="section">
      <div class="section-title">Tanque</div>
      <label>Radio (cm)</label><input name="tank_radius_cm" value="{{tank_radius_cm}}" placeholder="45.0">
      <div class="gap"></div>
      <label>Longitud (cm)</label><input name="tank_length_cm" value="{{tank_length_cm}}" placeholder="120.0">
    </div>
    <div class="section">
      <div class="section-title">Seguridad OTA</div>
      <label>Clave HMAC-SHA256</label><input name="ota_hmac_key" value="{{ota_hmac_key}}" type="password" placeholder="s3cr3t-k3y">
    </div>
    <button type="submit">💾 Guardar y Reiniciar</button>
  </form>
</div>
</body>
</html>
)rawhtml";

static const char SUCCESS_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Guardado!</title>
  <style>
    body{font-family:'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;display:flex;justify-content:center;align-items:center}
    .card{background:#1e293b;border-radius:16px;padding:48px;text-align:center;box-shadow:0 25px 50px rgba(0,0,0,.5)}
    .icon{font-size:3rem;margin-bottom:16px}
    h1{color:#4ade80;font-size:1.5rem}
    p{color:#94a3b8;margin-top:8px}
  </style>
</head>
<body>
<div class="card">
  <div class="icon">✅</div>
  <h1>¡Configuración guardada!</h1>
  <p>El dispositivo se reiniciará en 3 segundos...</p>
</div>
</body>
</html>
)rawhtml";

// Replace {{placeholders}} in HTML with current config values
static String buildPage() {
    String html = FPSTR(CONFIG_PAGE);
    html.replace("{{apn}}",            config.apn);
    html.replace("{{apn_user}}",       config.apn_user);
    html.replace("{{apn_pass}}",       "");
    html.replace("{{wifi_ssid}}",      config.wifi_ssid);
    html.replace("{{wifi_pass}}",      "");
    html.replace("{{mqtt_broker}}",    config.mqtt_broker);
    html.replace("{{mqtt_port}}",      String(config.mqtt_port));
    html.replace("{{mqtt_user}}",      config.mqtt_user);
    html.replace("{{mqtt_pass}}",      "");
    html.replace("{{device_id}}",      config.device_id);
    html.replace("{{tank_radius_cm}}", String(config.tank_radius_cm, 1));
    html.replace("{{tank_length_cm}}", String(config.tank_length_cm, 1));
    html.replace("{{ota_hmac_key}}",   "");
    return html;
}

void initWiFiAPProvisioning() {
    // Nothing to do here – setup happens inside runWiFiAPProvisioning
}

bool runWiFiAPProvisioning(unsigned long timeoutMs) {
    Serial.println("[WiFi-AP] Starting captive portal...");
    _configSaved = false;

    String ssid = "GLP-CONFIG-" + config.device_id;
    WiFi.softAP(ssid.c_str());
    Serial.printf("[WiFi-AP] SSID: %s  IP: %s\n", ssid.c_str(),
                  WiFi.softAPIP().toString().c_str());

    // Captive-portal DNS: redirect everything to 192.168.4.1
    _dns.start(53, "*", WiFi.softAPIP());

    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", buildPage());
    });

    // Some OSes hit /generate_204 or /hotspot-detect.html for captive detection
    _server.onNotFound([](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });

    _server.on("/save", HTTP_POST, [](AsyncWebServerRequest* req) {
        auto get = [&](const char* key) -> String {
            return req->hasParam(key, true) ? req->getParam(key, true)->value() : "";
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
        config.device_id      = get("device_id");
        config.tank_radius_cm = get("tank_radius_cm").toFloat();
        config.tank_length_cm = get("tank_length_cm").toFloat();
        config.ota_hmac_key   = get("ota_hmac_key");

        saveConfig();
        _configSaved = true;

        req->send(200, "text/html", FPSTR(SUCCESS_PAGE));
        Serial.println("[WiFi-AP] Config saved via web form. Rebooting...");
    });

    _server.begin();

    unsigned long start = millis();
    while ((millis() - start) < timeoutMs) {
        _dns.processNextRequest();
        if (_configSaved) {
            delay(3000);          // let success page render
            _server.end();
            _dns.stop();
            WiFi.softAPdisconnect(true);
            return true;
        }
        delay(10);
    }

    _server.end();
    _dns.stop();
    WiFi.softAPdisconnect(true);
    return false;
}
