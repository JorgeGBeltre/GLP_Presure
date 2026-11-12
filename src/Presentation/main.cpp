#include <Arduino.h>
#include <WiFi.h>

#include "Infrastructure/Config/BoardConfig.h"
#include "Infrastructure/Config/NvsConfigRepository.h"
#include "Infrastructure/Config/RemoteConfigParser.h"
#include "Infrastructure/Config/nvs_config.h"

#include "Domain/Services/CalibratingSensorReader.h"
#include "Infrastructure/Button/GpioButton.h"
#include "Infrastructure/Gps/TinyGpsProvider.h"
#include "Infrastructure/Indicator/GpioIndicator.h"
#include "Infrastructure/Logging/SerialLogger.h"
#include "Infrastructure/Messaging/Mqttv5Transport.h"
#include "Infrastructure/Messaging/mqtt_client.h"
#include "Infrastructure/Net/CellularLink.h"
#include "Infrastructure/Net/LanZeroconfServer.h"
#include "Infrastructure/Net/WifiLink.h"
#include "Infrastructure/Net/sim_module.h"
#include "Infrastructure/Ota/Mqttv5OtaService.h"
#include "Infrastructure/Provisioning/ProvisioningPortals.h"
#include "Infrastructure/Sensors/Esp32RawSensorReader.h"
#include "Infrastructure/Sensors/Mpu6050TiltProvider.h"
#include "Infrastructure/Serialization/ArduinoJsonPayloadSerializer.h"
#include "Infrastructure/System/EspDeviceRepository.h"
#include "Infrastructure/System/EspSystemControl.h"
#include "Infrastructure/Time/ArduinoClock.h"
#include "Infrastructure/Time/NtpTimeProvider.h"

#include "Application/Controllers/FirmwareController.h"
#include "Application/Messaging/MqttTopics.h"
#include "Application/Services/ConfigService.h"
#include "Application/Services/ConnectivityService.h"
#include "Application/Services/TelemetryService.h"

using namespace glp;

static infra::ArduinoClock                 g_clock;
static infra::SerialLogger                 g_logger;
static infra::EspSystemControl             g_system;
static infra::GpioIndicator                g_indicator{board::LED_PIN};
static infra::GpioButton                   g_button{board::BOOT_BTN_PIN, 50, 5000};
static infra::NvsConfigRepository          g_configRepo;
static infra::BleProvisioningPortal        g_blePortal;
static infra::WifiApProvisioningPortal     g_apPortal;
static infra::Esp32RawSensorReader         g_rawSensors;
static infra::TinyGpsProvider              g_gps;
static infra::Mpu6050TiltProvider          g_tilt;
static infra::CellularLink                 g_cellular;
static infra::Mqttv5Transport              g_transport;
static infra::Mqttv5OtaService             g_ota;
static infra::EspDeviceRepository          g_deviceRepo;
static infra::NtpTimeProvider              g_timeProvider;
static infra::ArduinoJsonPayloadSerializer g_serializer;
static infra::LanZeroconfServer            g_lanServer{g_indicator, g_configRepo, 8081};

static app::ConfigService                  g_configService{g_configRepo};
static infra::WifiLink                     g_wifi{g_configService.config()};
static domain::CalibratingSensorReader     g_sensors{g_rawSensors, g_configService.config()};
static app::ConnectivityService            g_connectivity{g_wifi, g_cellular, g_transport, g_logger};

static app::TelemetryService*              g_telemetry  = nullptr;
static app::FirmwareController*            g_controller = nullptr;
static volatile bool                       g_pendingFactoryUnlink = false;

static void executeFactoryUnlinkToQuickPairing() {
    Serial.println("[UNLINK] Ejecutando borrado de credenciales de cuenta y reinicio en Quick Pairing...");
    resetConfig();
    WiFi.disconnect(true, true);
    delay(250);
    infra::EspSystemControl::markGracefulReboot();
    ESP.restart();
}

// Flujo de emparejamiento de 2 etapas para GLP_Presure:
// - Etapa 1: Quick Pairing (BLE GATT GLP-<DeviceID> + SmartConfig ESP-Touch)
// - Etapa 2: SoftAP GLP-CONFIG-<DeviceID> (tras 2ª pulsación larga de 5s o fallback)
static void runPairingWorkflow(bool isInitialBoot) {
    infra::EspSystemControl::setWatchdogPaused(true);
    probeSimProvisioningState();

    Serial.println("[PAIRING] Entrando en Etapa 1: Quick Pairing (BLE + SmartConfig)...");
    QuickPairingResult res = g_blePortal.runInteractive(
        isInitialBoot ? 120000UL : 180000UL,
        g_indicator,
        g_button
    );

    if (res == QuickPairingResult::Provisioned) {
        Serial.println("[PAIRING] Credenciales recibidas en Quick Pairing. Reiniciando...");
        delay(400);
        infra::EspSystemControl::markGracefulReboot();
        ESP.restart();
        return;
    }

    if (res == QuickPairingResult::CancelledByButton) {
        Serial.println("[PAIRING] Emparejamiento cancelado por pulsación corta.");
        infra::EspSystemControl::setWatchdogPaused(false);
        return;
    }

    if (res == QuickPairingResult::SwitchToSoftAp || isInitialBoot) {
        Serial.println("[PAIRING] Entrando en Etapa 2: SoftAP GLP-CONFIG-<DeviceID>...");
        if (g_apPortal.runInteractive(300000UL, g_indicator, g_button)) {
            delay(400);
            infra::EspSystemControl::markGracefulReboot();
            ESP.restart();
            return;
        }
    }
    infra::EspSystemControl::setWatchdogPaused(false);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    infra::EspSystemControl::initBootAndWatchdog();
    Serial.printf("[BOOT] GLP boot #%lu (unexpected=%lu, wdt=%u, reason=%s)\n",
                  (unsigned long)infra::EspSystemControl::bootCount(),
                  (unsigned long)infra::EspSystemControl::unexpectedReboots(),
                  (unsigned)infra::EspSystemControl::wdtOffences(),
                  infra::EspSystemControl::resetReason());

    initConfig();
    g_button.init();

    setMqttUnlinkVerifiedCallback([]() {
        g_pendingFactoryUnlink = true;
    });
    g_lanServer.setUnlinkCallback([]() {
        g_pendingFactoryUnlink = true;
    });

    bool provisioned = g_configService.load();
    g_indicator.setSledOnline(g_configService.config().sledOnline);

    bool forceReprovision = (digitalRead(board::BOOT_BTN_PIN) == LOW);
    if (!provisioned || !g_configService.isProvisioned() || forceReprovision) {
        if (forceReprovision) {
            Serial.println("[BOOT] Botón BOOT presionado -> iniciando Quick Pairing...");
            resetConfig();
        }
        runPairingWorkflow(true);
        g_configService.load();
    }

    g_rawSensors.begin();
    g_gps.begin();
    g_tilt.begin();

    std::string devId = !g_configService.config().deviceId.empty()
                            ? g_configService.config().deviceId
                            : g_deviceRepo.identity().chipId;
    app::MqttTopics topics = app::MqttTopics::forDevice(devId);

    static app::TelemetryService telemetry{
        g_sensors, g_gps, g_tilt, g_connectivity, g_ota,
        g_deviceRepo, g_timeProvider, g_serializer, g_clock,
        g_configService, topics, 30000UL};
    g_telemetry = &telemetry;

    std::vector<app::FirmwareController::ProvisioningStep> steps = {
        {&g_blePortal, 120000UL},
        {&g_apPortal,  300000UL}
    };

    static app::FirmwareController controller{
        g_configService, std::move(steps), g_connectivity, telemetry,
        g_gps, g_ota, g_indicator, g_system, g_clock, g_logger};
    g_controller = &controller;

    g_controller->setup(false);
}

void loop() {
    infra::EspSystemControl::heartbeat();

    if (g_pendingFactoryUnlink) {
        g_pendingFactoryUnlink = false;
        executeFactoryUnlinkToQuickPairing();
        return;
    }

    auto btnEv = g_button.update();
    if (btnEv == domain::ButtonEvent::LongPress) {
        Serial.println("[BTN] Pulsación larga (5.0s) detectada -> Iniciando Quick Pairing...");
        runPairingWorkflow(false);
    }

    g_indicator.update();
    g_lanServer.loop(WiFi.status() == WL_CONNECTED);
    if (g_controller) {
        g_controller->tick();
    }
}
