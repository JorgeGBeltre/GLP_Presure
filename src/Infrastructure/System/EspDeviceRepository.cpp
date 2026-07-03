#include "Infrastructure/System/EspDeviceRepository.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_mac.h>

#include "Application/Version.h"
#include "Infrastructure/Net/sim_module.h"   // getSignalRSSI()

namespace glp::infra {

domain::DeviceIdentity EspDeviceRepository::identity() {
    domain::DeviceIdentity id;

    const uint64_t chip = ESP.getEfuseMac();
    char chipBuf[17];
    snprintf(chipBuf, sizeof(chipBuf), "%04X%08X",
             (uint16_t)(chip >> 32), (uint32_t)chip);
    id.chipId   = chipBuf;
    id.chipType = ESP.getChipModel();

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    id.macAddress = macBuf;

    id.firmwareVersion = app::kFirmwareVersion;
    id.signalQuality   = getSignalRSSI();
    // imei/imsi/simOperator: pendientes de exponer desde el módem (opcional).
    return id;
}

domain::SystemMetrics EspDeviceRepository::metrics() {
    domain::SystemMetrics m;
    m.rssi     = WiFi.RSSI();
    m.freeHeap = ESP.getFreeHeap();
    m.uptime   = millis();
    return m;
}

} // namespace glp::infra
