#include <gtest/gtest.h>

#include "Application/Services/ConfigService.h"
#include "support/Fakes.h"

using namespace glp;
using namespace glp::testing;

TEST(ConfigService, LoadsStoredConfigAndProvisionedFlag) {
    FakeConfigRepository repo;
    repo.loadable = true;
    repo.provisioned = true;
    repo.stored.deviceId = "GLP-001";
    repo.stored.tankRadiusCm = 45.0f;

    app::ConfigService svc{repo};
    EXPECT_TRUE(svc.load());
    EXPECT_EQ(svc.config().deviceId, "GLP-001");
    EXPECT_TRUE(svc.isProvisioned());
}

TEST(ConfigService, TankDimensionsConvertCmToMm) {
    FakeConfigRepository repo;
    app::ConfigService svc{repo};
    svc.mutableConfig().tankRadiusCm = 45.0f;
    svc.mutableConfig().tankLengthCm = 120.0f;

    const auto dims = svc.tankDimensions();
    EXPECT_FLOAT_EQ(dims.radiusMm, 450.0f);
    EXPECT_FLOAT_EQ(dims.lengthMm, 1200.0f);
}

TEST(ConfigService, ApplyRemoteUpdatePersistsChangedFields) {
    FakeConfigRepository repo;
    app::ConfigService svc{repo};

    app::RemoteConfigUpdate update;
    update.tankRadiusCm = 50.0f;
    update.mqttPort = 8883;

    EXPECT_TRUE(svc.applyRemoteUpdate(update));
    EXPECT_FLOAT_EQ(svc.config().tankRadiusCm, 50.0f);
    EXPECT_EQ(svc.config().mqttPort, 8883);
    EXPECT_GE(repo.saveCount, 1);
}

TEST(ConfigService, EmptyRemoteUpdateDoesNotPersist) {
    FakeConfigRepository repo;
    app::ConfigService svc{repo};

    EXPECT_FALSE(svc.applyRemoteUpdate(app::RemoteConfigUpdate{}));
    EXPECT_EQ(repo.saveCount, 0);
}
