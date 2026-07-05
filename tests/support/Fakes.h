#pragma once

// Dobles de prueba (hand-rolled, sin GMock) para las interfaces de dominio y de
// aplicación. Sólo dependen de cabeceras puras (Domain/Application), así que
// compilan y enlazan en host. Constituyen la biblioteca `glp_mocks`.

#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "Application/Serialization/IPayloadSerializer.h"
#include "Domain/Interfaces/IClock.h"
#include "Domain/Interfaces/IConfigRepository.h"
#include "Domain/Interfaces/IDeviceRepository.h"
#include "Domain/Interfaces/ITimeProvider.h"
#include "Domain/Interfaces/IGpsProvider.h"
#include "Domain/Interfaces/IIndicator.h"
#include "Domain/Interfaces/ILogger.h"
#include "Domain/Interfaces/INetworkLink.h"
#include "Domain/Interfaces/IOtaService.h"
#include "Domain/Interfaces/IProvisioningPortal.h"
#include "Domain/Interfaces/IRawSensorReader.h"
#include "Domain/Interfaces/ISensorReader.h"
#include "Domain/Interfaces/ITiltProvider.h"
#include "Domain/Interfaces/ISystemControl.h"
#include "Domain/Interfaces/ITelemetryTransport.h"

namespace glp::testing {

using namespace glp::domain;

class FakeConfigRepository : public IConfigRepository {
public:
    DeviceConfig stored;
    bool         loadable    = false;
    bool         provisioned = false;
    bool         erased      = false;
    int          saveCount   = 0;

    bool load(DeviceConfig& out) override { out = stored; return loadable; }
    bool save(const DeviceConfig& cfg) override { stored = cfg; ++saveCount; return true; }
    bool erase() override { erased = true; return true; }
    bool isProvisioned() override { return provisioned; }
};

class FakeSensorReader : public ISensorReader {
public:
    SensorSample next;
    bool         began = false;

    void begin() override { began = true; }
    SensorSample read() override { return next; }
};

class FakeRawSensorReader : public IRawSensorReader {
public:
    RawSensorSample next;
    bool            began = false;
    void begin() override { began = true; }
    RawSensorSample read() override { return next; }
};

class FakeTiltProvider : public ITiltProvider {
public:
    TiltReading next;
    bool        began = false;
    void begin() override { began = true; }
    TiltReading read() override { return next; }
};

class FakeGpsProvider : public IGpsProvider {
public:
    GpsFix fix;
    int    feeds = 0;

    void begin() override {}
    void feed() override { ++feeds; }
    GpsFix location() override { return fix; }
};

class FakeTransport : public ITelemetryTransport {
public:
    bool connected = false;
    std::vector<std::pair<std::string, std::string>> published;
    std::vector<std::string> subscriptions;
    MessageHandler handler;
    int loops = 0;

    bool connect() override { connected = true; return true; }
    void disconnect() override { connected = false; }
    bool isConnected() const override { return connected; }
    bool publish(const std::string& t, const std::string& p) override {
        published.push_back({t, p});
        return true;
    }
    void subscribe(const std::string& t) override { subscriptions.push_back(t); }
    void onMessage(MessageHandler h) override { handler = std::move(h); }
    void loop() override { ++loops; }
};

class FakeOtaService : public IOtaService {
public:
    bool inProgress = false;
    int  handles    = 0;

    void begin() override {}
    void handle() override { ++handles; }
    bool isInProgress() override { return inProgress; }
};

class FakeNetworkLink : public INetworkLink {
public:
    LinkType t;
    bool     configured    = true;
    bool     connected     = false;
    bool     connectResult = true;  // qué devuelve connect()
    int      rssiVal       = -70;

    explicit FakeNetworkLink(LinkType type) : t(type) {}
    LinkType type() const override { return t; }
    bool isConfigured() override { return configured; }
    bool connect() override { connected = connectResult; return connected; }
    bool isConnected() override { return connected; }
    int  rssi() override { return rssiVal; }
    std::string ipAddress() override { return "192.168.0.42"; }
};

class FakeDeviceRepository : public IDeviceRepository {
public:
    DeviceIdentity id;
    SystemMetrics  m;
    DeviceIdentity identity() override { return id; }
    SystemMetrics  metrics() override { return m; }
};

class FakeTimeProvider : public ITimeProvider {
public:
    std::string ts = "2026-07-03T12:00:00.000Z";
    bool synced = false;
    bool sync() override { synced = true; return true; }
    std::string isoTimestamp() override { return ts; }
};

class FakeProvisioningPortal : public IProvisioningPortal {
public:
    bool          result = false; // qué devuelve run()
    int           runs   = 0;
    unsigned long lastTimeout = 0;

    explicit FakeProvisioningPortal(bool r = false) : result(r) {}
    bool run(unsigned long timeoutMs) override {
        ++runs;
        lastTimeout = timeoutMs;
        return result;
    }
};

class FakeIndicator : public IIndicator {
public:
    std::vector<Status> shown;
    Status last() const { return shown.empty() ? Status::Error : shown.back(); }
    void show(Status s) override { shown.push_back(s); }
};

class FakeClock : public IClock {
public:
    unsigned long t = 0;
    unsigned long slept = 0;

    unsigned long nowMs() override { return t; }
    void sleepMs(unsigned long ms) override { slept += ms; }
};

class FakeSystemControl : public ISystemControl {
public:
    int reboots = 0;
    void reboot() override { ++reboots; }
};

class FakeLogger : public ILogger {
public:
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
};

// Serializador de prueba: no genera JSON real, sólo un marcador por tipo, para
// poder verificar QUE se publicó y EN qué tópico.
class FakePayloadSerializer : public glp::app::IPayloadSerializer {
public:
    std::string telemetry(const glp::app::TelemetrySnapshot&) override { return "telemetry"; }
};

} // namespace glp::testing
