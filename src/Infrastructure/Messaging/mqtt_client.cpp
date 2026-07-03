// TINY_GSM_MODEM_SIM7600 is defined via build_flags in platformio.ini
#include <TinyGsmClient.h>
#include "ESP32_MQTTv5.h"
#include <ArduinoJson.h>

#include "mqtt_client.h"
#include "Infrastructure/Config/nvs_config.h"
#include "Infrastructure/Ota/ota_manager.h"
#include "Infrastructure/Net/sim_module.h"       // TinyGsmClientBridge& getSimClient()
#include "Infrastructure/Net/NetworkClient.h"    // getActiveNetworkClient() (WiFi o celular)

// Forward declare the SIM layer's client accessor
TinyGsmClientBridge& getSimClient();

#define FIRMWARE_VERSION "1.0.0"

// Expose mqtt instance so ota_manager.cpp can use it
ESP32_MQTTv5 mqtt;

// Sumidero de mensajes entrantes (lo fija Mqttv5Transport). Si nadie lo registra
// los mensajes simplemente se ignoran.
static std::function<void(const String&, const String&)> g_messageSink;

void setMqttMessageSink(std::function<void(const String&, const String&)> sink) {
    g_messageSink = std::move(sink);
}

void initMQTT() {
    // begin() sobre el enlace activo: WiFiClient (WiFi) o TinyGsmClientBridge (SIM)
    mqtt.begin(glp::infra::getActiveNetworkClient(), config.mqtt_broker.c_str(), config.mqtt_port);
    mqtt.setKeepAlive(60);
    mqtt.setCleanStart(false);                       // preserve session for QoS1 queuing
    mqtt.setAutoReconnect(true, 1000, 60000);
    mqtt.setSessionExpiry(3600);
    mqtt.setBufferSize(4096);                        // increased for OTA chunks

    // --- Will Message: offline alert ---
    MQTTProperties willProps;
    willProps.setContentType("application/json");
    willProps.setWillDelay(5);
    String willTopic = "glp/status/" + config.device_id;
    String willMsg   = "{\"status\":\"offline\",\"device_id\":\"" + config.device_id + "\"}";
    mqtt.setWill(willTopic, willMsg, 1, true, &willProps);

    // --- On connect: subscribe to config topic + init OTA ---
    mqtt.onConnect([&]() {
        Serial.println("[MQTT] Connected to broker!");

        String configTopic = "glp/config/" + config.device_id;
        mqtt.subscribe(configTopic, 1);
        Serial.printf("[MQTT] Subscribed to %s\n", configTopic.c_str());

        // OTA must be initialized INSIDE onConnect() — per library examples
        initOTA();
    });

    // --- On disconnect ---
    mqtt.onDisconnect([](MQTTReasonCode rc) {
        Serial.printf("[MQTT] Disconnected (0x%02X)\n", (uint8_t)rc);
    });

    // --- On error ---
    mqtt.onError([](MQTTReasonCode rc, const String& msg) {
        Serial.printf("[MQTT] Error 0x%02X: %s\n", (uint8_t)rc, msg.c_str());
    });

    // --- On message: reenvía a la capa de Application vía el sumidero ---
    mqtt.onMessage([](const MQTTMessage& msg) {
        Serial.printf("[MQTT] Incoming: %s\n", msg.topic.c_str());
        if (g_messageSink) g_messageSink(msg.topic, msg.payload);
    });
}

void connectMQTT() {
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
    // Correct signature: publish(topic, payload, qos, retain, props*)
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
