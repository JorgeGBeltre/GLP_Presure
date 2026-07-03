#include <gtest/gtest.h>

#include "Application/Services/TelemetryService.h"
#include "support/Fakes.h"

using namespace glp;
using namespace glp::testing;
using domain::LinkType;

namespace {
struct Fixture {
    FakeConfigRepository  repo;
    app::ConfigService    config{repo};
    FakeSensorReader      sensors;
    FakeGpsProvider       gps;
    FakeNetworkLink       wifi{LinkType::Wifi};
    FakeNetworkLink       cellular{LinkType::Cellular};
    FakeTransport         transport;
    FakeLogger            log;
    app::ConnectivityService conn{wifi, cellular, transport, log};
    FakeOtaService        ota;
    FakeDeviceRepository  device;
    FakeTimeProvider      time;
    FakePayloadSerializer serializer;
    FakeClock             clock;

    app::TelemetryService svc{sensors, gps, conn, ota, device, time, serializer,
                              clock, config, app::MqttTopics::forDevice("chip"), 30000};

    Fixture() {
        transport.connected = true;
        config.mutableConfig().deviceId     = "chip";
        config.mutableConfig().tankRadiusCm = 45.0f;
        config.mutableConfig().tankLengthCm = 120.0f;
    }

    int count(const std::string& payload) const {
        int n = 0;
        for (const auto& p : transport.published)
            if (p.second == payload) ++n;
        return n;
    }
};
} // namespace

TEST(TelemetryService, PublishesEvery30sAfterSeeding) {
    Fixture f;
    f.clock.t = 0;     f.svc.loop();
    EXPECT_TRUE(f.transport.published.empty());

    f.clock.t = 30000; f.svc.loop();
    EXPECT_EQ(f.count("telemetry"), 1);
    EXPECT_EQ(f.transport.published[0].first, "glp/telemetria/chip");

    f.clock.t = 60000; f.svc.loop();
    EXPECT_EQ(f.count("telemetry"), 2);
}

TEST(TelemetryService, SuppressedWhenOtaInProgress) {
    Fixture f;
    f.ota.inProgress = true;
    EXPECT_FALSE(f.svc.publishTelemetry());
    EXPECT_TRUE(f.transport.published.empty());
}

TEST(TelemetryService, SuppressedWhenDisconnected) {
    Fixture f;
    f.transport.connected = false;
    EXPECT_FALSE(f.svc.publishTelemetry());
    EXPECT_TRUE(f.transport.published.empty());
}

TEST(TelemetryService, FeedsGpsBeforePublishing) {
    Fixture f;
    EXPECT_TRUE(f.svc.publishTelemetry());
    EXPECT_GE(f.gps.feeds, 1);
}
