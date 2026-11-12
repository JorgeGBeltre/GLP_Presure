// TINY_GSM_MODEM_SIM7600 is defined via build_flags in platformio.ini
#include <TinyGsmClient.h>
#include "ESP32_MQTTv5.h"
#include <ArduinoJson.h>
#include <cstring>

#include "mqtt_client.h"
#include "Infrastructure/Config/nvs_config.h"
#include "Infrastructure/Ota/ota_manager.h"
#include "Infrastructure/Net/sim_module.h"
#include "Infrastructure/Net/NetworkClient.h"

TinyGsmClientBridge& getSimClient();

#define FIRMWARE_VERSION GLP_FIRMWARE_VERSION

ESP32_MQTTv5 mqtt;

static std::function<void(const String&, const String&)> g_messageSink;
static std::function<void()> g_unlinkVerifiedCb;

void setMqttMessageSink(std::function<void(const String&, const String&)> sink) {
    g_messageSink = std::move(sink);
}

void setMqttUnlinkVerifiedCallback(std::function<void()> cb) {
    g_unlinkVerifiedCb = std::move(cb);
}

static bool tryVerifyAuthUnlinkReason(const String& reasonStr) {
    const char* prefix = "ACCOUNT_UNLINKED:";
    const size_t prefixLen = std::strlen(prefix);
    if (reasonStr.length() < prefixLen + 32) return false;
    int idx = reasonStr.indexOf(prefix);
    if (idx < 0) return false;
    String proof = reasonStr.substring(idx + (int)prefixLen, idx + (int)prefixLen + 32);
    return verifyUnlinkProof(config.device_key.c_str(), config.device_id.c_str(), proof.c_str());
}

void initMQTT() {
    mqtt.begin(glp::infra::getActiveNetworkClient(), config.mqtt_broker.c_str(), config.mqtt_port);
    mqtt.setKeepAlive(60);
    mqtt.setCleanStart(false);
    mqtt.setAutoReconnect(false); // Reconexión exponencial centralizada en FirmwareController
    mqtt.setSessionExpiry(3600);
    mqtt.setBufferSize(4096);

    MQTTProperties willProps;
    willProps.setContentType("application/json");
    willProps.setWillDelay(5);
    String willTopic = "glp/status/" + config.device_id;
    String willMsg   = "{\"status\":\"offline\",\"device_id\":\"" + config.device_id + "\"}";
    mqtt.setWill(willTopic, willMsg, 1, true, &willProps);

    mqtt.onConnect([&]() {
        Serial.println("[MQTT] Connected to broker!");

        String configTopic = "glp/config/" + config.device_id;
        mqtt.subscribe(configTopic, 1);
        Serial.printf("[MQTT] Subscribed to %s\n", configTopic.c_str());

        String unlinkTopic = "devices/" + config.device_id + "/unlink/command";
        mqtt.subscribe(unlinkTopic, 1);
        Serial.printf("[MQTT] Subscribed to %s\n", unlinkTopic.c_str());

        initOTA();
        MQTTProperties reconnectWillProps;
        reconnectWillProps.setContentType("application/json");
        reconnectWillProps.setWillDelay(5);
        mqtt.setWill("glp/status/" + config.device_id,
                     "{\"status\":\"offline\",\"device_id\":\"" + config.device_id + "\"}",
                     1, true, &reconnectWillProps);
    });

    mqtt.onDisconnect([](MQTTReasonCode rc) {
        const String& reason = mqtt.lastReasonString();
        Serial.printf("[MQTT] Disconnected (0x%02X) reason='%s'\n", (uint8_t)rc, reason.c_str());
        if ((uint8_t)rc == 0x87 && tryVerifyAuthUnlinkReason(reason)) {
            Serial.println("[UNLINK] Desvinculación diferida autenticada en DISCONNECT 0x87.");
            if (g_unlinkVerifiedCb) g_unlinkVerifiedCb();
        }
    });

    mqtt.onError([](MQTTReasonCode rc, const String& msg) {
        const String& reason = mqtt.lastReasonString();
        Serial.printf("[MQTT] Error 0x%02X: %s (reason='%s')\n", (uint8_t)rc, msg.c_str(), reason.c_str());
        if ((uint8_t)rc == 0x87 && (tryVerifyAuthUnlinkReason(reason) || tryVerifyAuthUnlinkReason(msg))) {
            Serial.println("[UNLINK] Desvinculación diferida autenticada en CONNACK 0x87.");
            if (g_unlinkVerifiedCb) g_unlinkVerifiedCb();
        }
    });

    mqtt.onMessage([](const MQTTMessage& msg) {
        Serial.printf("[MQTT] Incoming: %s\n", msg.topic.c_str());
        String unlinkTopic = "devices/" + config.device_id + "/unlink/command";
        if (msg.topic == unlinkTopic) {
            StaticJsonDocument<256> doc;
            if (deserializeJson(doc, msg.payload) == DeserializationError::Ok) {
                const char* proof = doc["unlink_proof"] | doc["proof"];
                if (proof && verifyUnlinkProof(config.device_key.c_str(), config.device_id.c_str(), proof)) {
                    String ackTopic = "devices/" + config.device_id + "/unlink/ack";
                    StaticJsonDocument<192> ackDoc;
                    ackDoc["device_id"] = config.device_id;
                    ackDoc["status"]    = "UNLINKED";
                    if (doc.containsKey("command_id")) ackDoc["command_id"] = doc["command_id"];
                    String ackJson;
                    serializeJson(ackDoc, ackJson);
                    mqtt.publish(ackTopic, ackJson, 1, false);
                    Serial.println("[UNLINK] Orden MQTT de desvinculación verificada criptográficamente.");
                    if (g_unlinkVerifiedCb) g_unlinkVerifiedCb();
                } else {
                    Serial.println("[UNLINK] Rechazada orden MQTT con unlink_proof inválido.");
                }
            }
            return;
        }
        if (g_messageSink) g_messageSink(msg.topic, msg.payload);
    });
}

void connectMQTT() {
    if (!hasValidBrokerCredentials()) {
        return;
    }
    // Re-enlazar el cliente activo (WiFiClient <-> TinyGsmClientBridge) en cada failover
    mqtt.begin(glp::infra::getActiveNetworkClient(), config.mqtt_broker.c_str(), config.mqtt_port);
    Serial.printf("[MQTT] Connecting to %s:%d as %s...\n",
                  config.mqtt_broker.c_str(), config.mqtt_port, config.device_id.c_str());
    mqtt.connect(config.device_id, config.mqtt_user, config.mqtt_pass);
}

void publishTelemetry(const String& jsonPayload) {
    if (!mqtt.connected()) return;

    MQTTProperties props;
    props.setContentType("application/json");
    props.setMessageExpiry(120);
    props.addUserProperty("fw_version", FIRMWARE_VERSION);
    props.addUserProperty("uptime", String(millis() / 1000));

    String topic = "glp/telemetria/" + config.device_id;
    mqtt.publish(topic, jsonPayload, 1, false, &props);

    Serial.printf("[MQTT] Published %d bytes to %s\n",
                  jsonPayload.length(), topic.c_str());
}

void mqttLoop() {
    mqtt.loop();
}

bool isMQTTConnected() {
    return mqtt.connected();
}
