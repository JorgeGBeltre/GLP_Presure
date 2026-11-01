#include "Infrastructure/Net/LanZeroconfServer.h"
#include "Application/Services/CryptoUtils.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <cstdio>
#include <cstring>

namespace glp::infra {

LanZeroconfServer::LanZeroconfServer(domain::IIndicator& indicator,
                                     domain::IConfigRepository& configRepo,
                                     uint16_t port)
    : indicator_(indicator),
      configRepo_(configRepo),
      port_(port),
      server_(port) {}

void LanZeroconfServer::registerEndpoint(const char* uri) {
    server_.on(uri, HTTP_POST, [this, uri]() {
        String body = server_.arg("plain");
        char responseBuf[768] = {0};
        dispatchZeroconf(uri, body, responseBuf, sizeof(responseBuf));
        server_.send(200, "application/json", responseBuf);
    });
}

void LanZeroconfServer::startServer() {
    if (running_) return;

    domain::DeviceConfig devCfg;
    configRepo_.load(devCfg);
    const std::string devId = !devCfg.deviceId.empty() ? devCfg.deviceId : "GLP0001";

    char hostName[48];
    std::snprintf(hostName, sizeof(hostName), "eWeLink_%s", devId.c_str());
    if (MDNS.begin(hostName)) {
        MDNS.addService("_ewelink", "_tcp", port_);
        MDNS.addServiceTxt("_ewelink", "_tcp", "txtvers", "1");
        MDNS.addServiceTxt("_ewelink", "_tcp", "id", devId.c_str());
        MDNS.addServiceTxt("_ewelink", "_tcp", "type", "glp_sensor");
        MDNS.addServiceTxt("_ewelink", "_tcp", "apivers", "1");
    }

    registerEndpoint("/zeroconf/sled");
    registerEndpoint("/zeroconf/info");
    registerEndpoint("/zeroconf/signal_strength");
    registerEndpoint("/zeroconf/wifi");
    registerEndpoint("/zeroconf/ota_unlock");

    server_.begin();
    running_ = true;
    notifyMdnsStateChange();
    Serial.printf("[LAN] Servidor HTTP Zeroconf GLP iniciado en puerto %u (mDNS: %s.local)\n", port_, hostName);
}

void LanZeroconfServer::stopServer() {
    if (!running_) return;
    server_.stop();
    MDNS.end();
    running_ = false;
}

void LanZeroconfServer::loop(bool wifiConnected) {
    if (!wifiConnected) {
        if (running_) stopServer();
        return;
    }
    if (!running_) {
        startServer();
    }
    server_.handleClient();
}

void LanZeroconfServer::notifyMdnsStateChange() {
    if (!running_) return;

    ++sequence_;
    char seqBuf[16];
    std::snprintf(seqBuf, sizeof(seqBuf), "%u", sequence_);

    domain::DeviceConfig devCfg;
    configRepo_.load(devCfg);

    char plainData[220];
    std::snprintf(plainData, sizeof(plainData),
                  "{\"deviceid\":\"%s\",\"sledOnline\":\"%s\",\"tankR\":%.1f,\"tankL\":%.1f}",
                  devCfg.deviceId.c_str(),
                  indicator_.isSledOnline() ? "on" : "off",
                  devCfg.tankRadiusCm,
                  devCfg.tankLengthCm);

    MDNS.addServiceTxt("_ewelink", "_tcp", "seq", static_cast<const char*>(seqBuf));

    if (!devCfg.deviceKey.empty()) {
        uint8_t rawIv[16] = {0};
        std::snprintf(reinterpret_cast<char*>(rawIv), sizeof(rawIv), "%015u", sequence_);
        char ivB64[32] = {0};
        app::CryptoUtils::base64Encode(rawIv, 16, ivB64, sizeof(ivB64));
        char dataB64[360] = {0};
        if (app::CryptoUtils::encryptZeroconfPayload(
                devCfg.deviceKey.c_str(), ivB64, plainData, dataB64, sizeof(dataB64))) {
            MDNS.addServiceTxt("_ewelink", "_tcp", "encrypt", "true");
            MDNS.addServiceTxt("_ewelink", "_tcp", "iv", static_cast<const char*>(ivB64));
            MDNS.addServiceTxt("_ewelink", "_tcp", "data1", static_cast<const char*>(dataB64));
            return;
        }
    }

    MDNS.addServiceTxt("_ewelink", "_tcp", "data1", static_cast<const char*>(plainData));
}

void LanZeroconfServer::dispatchZeroconf(const char* uri, const String& rawBody, char* respBuf, size_t respBufSize) {
    auto writeError = [&](int errCode) {
        std::snprintf(respBuf, respBufSize,
                      "{\"sequence\":\"%u\",\"seq\":%u,\"error\":%d}",
                      sequence_, sequence_, errCode);
    };

    if (rawBody.length() == 0 || rawBody.length() > 1024) {
        writeError(400);
        return;
    }

    StaticJsonDocument<768> outerDoc;
    if (deserializeJson(outerDoc, rawBody) != DeserializationError::Ok) {
        writeError(400);
        return;
    }

    bool isEncrypted = outerDoc["encrypt"].as<bool>();
    char decryptedJson[512] = {0};
    StaticJsonDocument<512> innerDoc;
    JsonVariant dataObj;

    domain::DeviceConfig devCfg;
    configRepo_.load(devCfg);

    if (isEncrypted) {
        const char* ivB64 = outerDoc["iv"];
        const char* dataB64 = outerDoc["data"];
        if (!ivB64 || !dataB64 || devCfg.deviceKey.empty()) {
            writeError(400);
            return;
        }
        if (!app::CryptoUtils::decryptZeroconfPayload(devCfg.deviceKey.c_str(), ivB64, dataB64, decryptedJson, sizeof(decryptedJson))) {
            writeError(400);
            return;
        }
        if (deserializeJson(innerDoc, decryptedJson) != DeserializationError::Ok) {
            writeError(400);
            return;
        }
        dataObj = innerDoc.as<JsonVariant>();
    } else if (outerDoc["data"].is<JsonObject>()) {
        dataObj = outerDoc["data"];
    } else if (outerDoc["data"].is<const char*>()) {
        if (deserializeJson(innerDoc, outerDoc["data"].as<const char*>()) != DeserializationError::Ok) {
            writeError(400);
            return;
        }
        dataObj = innerDoc.as<JsonVariant>();
    } else {
        dataObj = outerDoc.as<JsonVariant>();
    }

    if (std::strcmp(uri, "/zeroconf/sled") == 0) {
        const char* sl = dataObj["sledOnline"];
        if (!sl) {
            writeError(400);
            return;
        }
        const bool en = (std::strcmp(sl, "on") == 0);
        indicator_.setSledOnline(en);
        configRepo_.saveSledOnline(en);
        notifyMdnsStateChange();
        writeError(0);
        return;
    }

    if (std::strcmp(uri, "/zeroconf/signal_strength") == 0) {
        ++sequence_;
        std::snprintf(respBuf, respBufSize,
                      "{\"sequence\":\"%u\",\"seq\":%u,\"error\":0,\"data\":{\"signalStrength\":%d}}",
                      sequence_, sequence_, (int)WiFi.RSSI());
        return;
    }

    if (std::strcmp(uri, "/zeroconf/info") == 0) {
        ++sequence_;
        std::snprintf(
            respBuf, respBufSize,
            "{\"sequence\":\"%u\",\"seq\":%u,\"error\":0,\"data\":{"
            "\"deviceid\":\"%s\",\"sledOnline\":\"%s\","
            "\"tankRadiusCm\":%.1f,\"tankLengthCm\":%.1f,\"rssi\":%d}}",
            sequence_, sequence_,
            devCfg.deviceId.c_str(),
            indicator_.isSledOnline() ? "on" : "off",
            devCfg.tankRadiusCm,
            devCfg.tankLengthCm,
            (int)WiFi.RSSI());
        return;
    }

    if (std::strcmp(uri, "/zeroconf/wifi") == 0) {
        const char* ssid = dataObj["ssid"];
        const char* pwd = dataObj["password"] | "";
        if (!ssid || std::strlen(ssid) == 0 || std::strlen(ssid) > 32) {
            writeError(400);
            return;
        }
        devCfg.wifiSsid = ssid;
        devCfg.wifiPass = pwd;
        configRepo_.save(devCfg);
        writeError(0);
        return;
    }

    if (std::strcmp(uri, "/zeroconf/ota_unlock") == 0) {
        otaUnlocked_ = true;
        writeError(0);
        return;
    }

    writeError(404);
}

} // namespace glp::infra

