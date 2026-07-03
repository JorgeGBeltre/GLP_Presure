#include <gtest/gtest.h>

#include "Application/Services/ConnectivityService.h"
#include "support/Fakes.h"

using namespace glp;
using namespace glp::testing;
using domain::LinkType;

namespace {
struct Fixture {
    FakeNetworkLink wifi{LinkType::Wifi};
    FakeNetworkLink cellular{LinkType::Cellular};
    FakeTransport   transport;
    FakeLogger      log;
    app::ConnectivityService conn{wifi, cellular, transport, log};
};
} // namespace

TEST(ConnectivityService, PrefersWifiWhenConfigured) {
    Fixture f;
    f.wifi.configured = true;
    f.wifi.connectResult = true;

    EXPECT_TRUE(f.conn.connect());
    EXPECT_EQ(f.conn.activeType(), LinkType::Wifi);
    EXPECT_TRUE(f.conn.isOnline());
    EXPECT_TRUE(f.transport.connected);
}

TEST(ConnectivityService, FallsBackToCellularWhenWifiUnconfigured) {
    Fixture f;
    f.wifi.configured = false;

    EXPECT_TRUE(f.conn.connect());
    EXPECT_EQ(f.conn.activeType(), LinkType::Cellular);
    EXPECT_TRUE(f.cellular.connected);
}

TEST(ConnectivityService, FallsBackToCellularWhenWifiFails) {
    Fixture f;
    f.wifi.configured = true;
    f.wifi.connectResult = false;   // WiFi configurado pero no conecta

    EXPECT_TRUE(f.conn.connect());
    EXPECT_EQ(f.conn.activeType(), LinkType::Cellular);
}

TEST(ConnectivityService, FailsWhenNoLinkAvailable) {
    Fixture f;
    f.wifi.configured = false;
    f.cellular.connectResult = false;

    EXPECT_FALSE(f.conn.connect());
    EXPECT_FALSE(f.conn.isOnline());
}

TEST(ConnectivityService, EnsureConnectedFailsWhenActiveLinkDown) {
    Fixture f;
    f.wifi.configured = false;
    ASSERT_TRUE(f.conn.connect());      // celular activo
    f.cellular.connected = false;        // el enlace cae
    EXPECT_FALSE(f.conn.ensureConnected());
}

TEST(ConnectivityService, EnsureConnectedReconnectsMqttWhenOnlyTransportDown) {
    Fixture f;
    f.wifi.configured = false;
    ASSERT_TRUE(f.conn.connect());
    f.transport.connected = false;       // sólo cae MQTT
    EXPECT_TRUE(f.conn.ensureConnected());
    EXPECT_TRUE(f.transport.connected);
}

TEST(ConnectivityService, ActiveRssiComesFromActiveLink) {
    Fixture f;
    f.wifi.configured = false;
    f.cellular.rssiVal = -55;
    ASSERT_TRUE(f.conn.connect());
    EXPECT_EQ(f.conn.activeRssi(), -55);
}
