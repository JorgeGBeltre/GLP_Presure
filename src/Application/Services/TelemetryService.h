#pragma once

#include "Application/Messaging/MqttTopics.h"
#include "Application/Serialization/IPayloadSerializer.h"
#include "Application/Services/ConfigService.h"
#include "Application/Services/ConnectivityService.h"
#include "Domain/Interfaces/IClock.h"
#include "Domain/Interfaces/IDeviceRepository.h"
#include "Domain/Interfaces/IGpsProvider.h"
#include "Domain/Interfaces/IOtaService.h"
#include "Domain/Interfaces/ISensorReader.h"
#include "Domain/Interfaces/ITimeProvider.h"
#include "Domain/Services/SensorFusion.h"

namespace glp::app {

/**
 * Publica telemetría cada `intervalMs` con el sobre {Device, Timestamp, Details}
 * (patrón Toour). Lee sensores + GPS, corre la fusión de dominio (Kalman +
 * volumen) y ensambla el envelope con identidad, conexión activa, timestamp y
 * métricas de sistema. Suprime si el transporte está desconectado o hay OTA.
 */
class TelemetryService {
public:
    TelemetryService(domain::ISensorReader& sensors,
                     domain::IGpsProvider& gps,
                     ConnectivityService& connectivity,
                     domain::IOtaService& ota,
                     domain::IDeviceRepository& device,
                     domain::ITimeProvider& time,
                     IPayloadSerializer& serializer,
                     domain::IClock& clock,
                     ConfigService& config,
                     MqttTopics topics,
                     unsigned long intervalMs = 30000)
        : sensors_(sensors), gps_(gps), conn_(connectivity), ota_(ota),
          device_(device), time_(time), serializer_(serializer), clock_(clock),
          config_(config), topics_(std::move(topics)), intervalMs_(intervalMs) {}

    void resetFilters() { fusion_.reset(); }
    void loop();
    bool publishTelemetry();

private:
    bool suppressed();

    domain::ISensorReader&      sensors_;
    domain::IGpsProvider&       gps_;
    ConnectivityService&        conn_;
    domain::IOtaService&        ota_;
    domain::IDeviceRepository&  device_;
    domain::ITimeProvider&      time_;
    IPayloadSerializer&         serializer_;
    domain::IClock&             clock_;
    ConfigService&              config_;
    MqttTopics                  topics_;
    unsigned long               intervalMs_;

    domain::SensorFusion fusion_;
    bool                 seeded_        = false;
    unsigned long        lastPublishMs_ = 0;
};

} // namespace glp::app
