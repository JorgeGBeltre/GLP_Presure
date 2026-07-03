#include <gtest/gtest.h>

#include "Application/Controllers/FirmwareController.h"
#include "support/Fakes.h"

using namespace glp;
using namespace glp::testing;
using domain::LinkType;
using State = app::FirmwareController::State;

namespace {
struct Fixture {
    FakeConfigRepository  repo;
    app::ConfigService    config{repo};

    FakeProvisioningPortal ble{false};
    FakeProvisioningPortal wifiAp{false};

    FakeNetworkLink wifi{LinkType::Wifi};
    FakeNetworkLink cellular{LinkType::Cellular};
    FakeTransport   transport;
    FakeLogger      log;
    app::ConnectivityService conn{wifi, cellular, transport, log};

    FakeSensorReader      sensors;
    FakeGpsProvider       gps;
    FakeOtaService        ota;
    FakeDeviceRepository  device;
    FakeTimeProvider      time;
    FakePayloadSerializer serializer;
    FakeClock             clock;
    app::TelemetryService telemetry{sensors, gps, conn, ota, device, time,
                                    serializer, clock, config,
                                    app::MqttTopics::forDevice("chip"), 30000};

    FakeIndicator     indicator;
    FakeSystemControl system;

    app::FirmwareController controller{
        config,
        { {&ble, 30000}, {&wifiAp, 300000} },
        conn, telemetry, gps, ota, indicator, system, clock, log};

    Fixture() {
        wifi.configured = false; // dispositivos sin WiFi provisionado -> celular
    }
};
} // namespace

TEST(FirmwareController, ProvisionedDeviceStartsConnecting) {
    Fixture f;
    f.repo.provisioned = true;
    f.controller.setup();
    EXPECT_EQ(f.controller.state(), State::Connecting);
}

TEST(FirmwareController, UnprovisionedDeviceStartsProvisioning) {
    Fixture f;
    f.repo.provisioned = false;
    f.controller.setup();
    EXPECT_EQ(f.controller.state(), State::Provisioning);
}

TEST(FirmwareController, ForceProvisioningOverridesProvisionedFlag) {
    Fixture f;
    f.repo.provisioned = true;
    f.controller.setup(/*forceProvisioning=*/true);
    EXPECT_EQ(f.controller.state(), State::Provisioning);
}

TEST(FirmwareController, ProvisioningRebootsAfterPortalTimeout) {
    Fixture f;
    f.controller.setup();
    f.controller.tick();
    EXPECT_EQ(f.ble.runs, 1);
    EXPECT_EQ(f.wifiAp.runs, 1);
    EXPECT_EQ(f.system.reboots, 1);
}

TEST(FirmwareController, ConnectingTransitionsToOperationalOnSuccess) {
    Fixture f;
    f.repo.provisioned = true;
    f.controller.setup();
    f.controller.tick();
    EXPECT_EQ(f.controller.state(), State::Operational);
    EXPECT_TRUE(f.conn.isOnline());
}

TEST(FirmwareController, OperationalPublishesTelemetryOnInterval) {
    Fixture f;
    f.repo.provisioned = true;
    f.controller.setup();
    f.clock.t = 0;      f.controller.tick(); // Connecting -> Operational
    f.clock.t = 1000;   f.controller.tick(); // siembra telemetría
    f.clock.t = 31000;  f.controller.tick(); // publica

    ASSERT_FALSE(f.transport.published.empty());
    EXPECT_EQ(f.transport.published.back().second, "telemetry");
    EXPECT_EQ(f.indicator.last(), domain::Status::Operational);
}

TEST(FirmwareController, OperationalFallsBackToConnectingWhenLinkDrops) {
    Fixture f;
    f.repo.provisioned = true;
    f.controller.setup();
    f.controller.tick();                // -> Operational (celular)
    ASSERT_EQ(f.controller.state(), State::Operational);

    f.cellular.connected = false;       // se cae el enlace celular
    f.transport.connected = false;
    f.controller.tick();
    EXPECT_EQ(f.controller.state(), State::Connecting);
}
