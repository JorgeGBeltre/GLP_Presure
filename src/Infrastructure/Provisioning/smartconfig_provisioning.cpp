#include "smartconfig_provisioning.h"

#include <WiFi.h>

#include "Infrastructure/Config/nvs_config.h"

bool runSmartConfigProvisioning(unsigned long timeoutMs) {
    Serial.println("[SmartConfig] EspTouch v1 activo — usar la app 'EspTouch'...");
    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // ESP-Touch v1 explícito (SC_TYPE_ESPTOUCH) para coincidir con el emisor.
    if (!WiFi.beginSmartConfig(SC_TYPE_ESPTOUCH)) {   // esp_smartconfig_start (nativo ESP-IDF)
        Serial.println("[SmartConfig] ERROR: no se pudo iniciar SmartConfig");
        return false;
    }

    const unsigned long start = millis();

    // 1) Esperar a que la app entregue SSID + clave.
    while (!WiFi.smartConfigDone()) {
        if (millis() - start > timeoutMs) {
            Serial.println("[SmartConfig] Timeout — sin credenciales.");
            WiFi.stopSmartConfig();
            return false;
        }
        delay(200);
    }

    // 2) Esperar a que asocie con la red (para confirmar SSID/PSK).
    while (WiFi.status() != WL_CONNECTED && (millis() - start) <= timeoutMs) {
        delay(200);
    }

    config.wifi_ssid = WiFi.SSID();
    config.wifi_pass = WiFi.psk();
    WiFi.stopSmartConfig();

    if (config.wifi_ssid.length() == 0) {
        Serial.println("[SmartConfig] SSID vacío tras EspTouch.");
        return false;
    }

    saveConfig();
    Serial.printf("[SmartConfig] WiFi provisionado: %s\n", config.wifi_ssid.c_str());
    return true;
}
