/**
 * main.cpp — Presentation / Composition Root
 *
 * Sistema IoT de Medición de GLP con ESP32.
 * Arquitectura: PROVISIONING -> CONNECTING -> OPERATIONAL (ver FirmwareController).
 *
 * Este archivo es el ÚNICO lugar donde se instancian implementaciones concretas
 * (Infrastructure) y se inyectan en los servicios de Application. No contiene
 * lógica de negocio: sólo cableado (dependency injection) y los hooks setup()/loop()
 * del framework Arduino.
 *
 * Hardware (sin cambios respecto a la versión previa):
 *   - Módulo 4G/LTE: SIM7600 (UART1 TX=26, RX=27, PWR=4)
 *   - GPS:           NMEA     (UART2 RX=16, TX=17)
 *   - Ultrasónico:   HC-SR04 / JSN-SR04T (TRIG=5, ECHO=18)
 *   - Presión:       ADC PIN=34   |  Temperatura: ADC PIN=35
 *   - LED estado:    GPIO=2       |  Botón BOOT:   GPIO=0 (pull-up)
 */

#include <Arduino.h>

#include "Application/Controllers/FirmwareController.h"
#include "Application/Services/ConfigService.h"
#include "Application/Services/ConnectivityService.h"
#include "Application/Services/TelemetryService.h"
#include "Application/Messaging/MqttTopics.h"

#include "Infrastructure/Config/BoardConfig.h"
#include "Infrastructure/Config/NvsConfigRepository.h"
#include "Infrastructure/Config/RemoteConfigParser.h"
#include "Infrastructure/System/EspDeviceRepository.h"
#include "Infrastructure/Time/NtpTimeProvider.h"
#include "Infrastructure/Gps/TinyGpsProvider.h"
#include "Infrastructure/Indicator/GpioIndicator.h"
#include "Infrastructure/Logging/SerialLogger.h"
#include "Infrastructure/Messaging/Mqttv5Transport.h"
#include "Infrastructure/Net/CellularLink.h"
#include "Infrastructure/Net/WifiLink.h"
#include "Infrastructure/Ota/Mqttv5OtaService.h"
#include "Infrastructure/Provisioning/ProvisioningPortals.h"
#include "Infrastructure/Sensors/Esp32SensorReader.h"
#include "Infrastructure/Serialization/ArduinoJsonPayloadSerializer.h"
#include "Infrastructure/System/EspSystemControl.h"
#include "Infrastructure/Time/ArduinoClock.h"

using namespace glp;

// ─── Constantes de hardware (pines en Infrastructure/Config/BoardConfig.h) ──
static constexpr int          LED_PIN       = glp::board::LED_PIN;
static constexpr int          BOOT_BTN_PIN  = glp::board::BOOT_BTN_PIN;
static constexpr unsigned long BOOT_HOLD_MS = 5000;
static constexpr unsigned long TELEMETRY_INTERVAL_MS = 30000;

// ─── Infrastructure (implementaciones concretas) ───────────────
static infra::ArduinoClock                 g_clock;
static infra::SerialLogger                 g_logger;
static infra::EspSystemControl             g_system;
static infra::GpioIndicator                g_indicator{LED_PIN};
static infra::NvsConfigRepository          g_configRepo;
static infra::Esp32SensorReader            g_sensors;
static infra::TinyGpsProvider              g_gps;
static infra::CellularLink                 g_cellular;
static infra::Mqttv5Transport              g_transport;
static infra::Mqttv5OtaService             g_ota;
static infra::ArduinoJsonPayloadSerializer g_serializer;
static infra::EspDeviceRepository          g_device;
static infra::NtpTimeProvider              g_time;
static infra::BleProvisioningPortal        g_ble;
static infra::WifiApProvisioningPortal     g_wifiAp;
static infra::SmartConfigPortal            g_smartConfig;

// El grafo de Application se construye en setup() (necesita el device_id ya
// cargado para derivar los tópicos MQTT). Se mantiene vía punteros estáticos.
static app::FirmwareController* g_controller = nullptr;

// ─── ¿Forzar provisioning? (BOOT mantenido >5s) ────────────────
static bool shouldForceProvisioning() {
    if (digitalRead(BOOT_BTN_PIN) == LOW) {
        const unsigned long start = millis();
        while (digitalRead(BOOT_BTN_PIN) == LOW) {
            if (millis() - start > BOOT_HOLD_MS) return true;
            delay(50);
        }
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

    g_configRepo.begin();
    g_sensors.begin();
    g_gps.begin();

    // Servicios de Application (static => viven toda la ejecución).
    static app::ConfigService config{g_configRepo};
    config.load();

    // Enlace WiFi: lee SSID/clave de la config viva (uplink preferido si existe).
    static infra::WifiLink wifi{config.config()};
    static app::ConnectivityService connectivity{wifi, g_cellular, g_transport, g_logger};

    const app::MqttTopics topics = app::MqttTopics::forDevice(config.config().deviceId);

    static app::TelemetryService telemetry{
        g_sensors, g_gps, connectivity, g_ota, g_device, g_time, g_serializer,
        g_clock, config, topics, TELEMETRY_INTERVAL_MS};

    g_time.sync();   // registra SNTP (UTC); resuelve en background al haber red

    // Config remota (glp/config/{id}): parsea -> aplica vía ConfigService ->
    // si cambió la geometría, reinicia los filtros de fusión.
    // config y telemetry son static locals (duración estática) => usables en el
    // lambda sin capturarlos; sólo `topics` (automático) se captura por valor.
    g_transport.onMessage(
        [topics](const std::string& topic, const std::string& payload) {
            if (topic != topics.config) return;
            if (auto update = infra::RemoteConfigParser::parse(payload)) {
                if (config.applyRemoteUpdate(*update)) {
                    telemetry.resetFilters();
                    Serial.println("[CFG] Config remota aplicada.");
                }
            }
        });

    // Transportes de provisioning en orden: BLE -> SoftAP -> SmartConfig.
    // BLE y SoftAP entregan toda la config (incluye WiFi); SmartConfig sólo WiFi.
    static app::FirmwareController controller{
        config,
        { {&g_ble, 30000}, {&g_wifiAp, 300000}, {&g_smartConfig, 120000} },
        connectivity, telemetry, g_gps, g_ota, g_indicator, g_system,
        g_clock, g_logger};

    controller.setup(shouldForceProvisioning());
    g_controller = &controller;
}

void loop() {
    if (g_controller) g_controller->tick();
    delay(10); // yield
}
