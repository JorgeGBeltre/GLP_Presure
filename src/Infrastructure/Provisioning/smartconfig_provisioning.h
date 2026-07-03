#pragma once
#include <Arduino.h>

// Provisioning de credenciales WiFi vía SmartConfig / EspTouch (esp_smartconfig,
// API nativa de ESP-IDF; se usa el wrapper de Arduino WiFi.beginSmartConfig()).
// Guarda SSID/clave en NVS. NO lleva la config de negocio (APN/MQTT/tanque): para
// eso usar BLE o el portal SoftAP.
bool runSmartConfigProvisioning(unsigned long timeoutMs = 120000);
