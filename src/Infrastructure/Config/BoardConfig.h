#pragma once

#include <cstdint>

/**
 * Mapa único de pines del PCB (ESP32-S3-WROOM-1-N16R8). Sólo lo usa
 * Infrastructure. Valores válidos para el S3 (evitan GPIO26–37 = flash/PSRAM,
 * strapping 3/45/46 y USB 19/20; ADC1 = GPIO1–10).
 *
 * ⚠️ Estos son valores por defecto seguros para el S3. Ajustar a tu cableado
 * real: son el ÚNICO lugar donde se definen los pines.
 */
namespace glp::board {

// Estado / control
constexpr uint8_t LED_PIN      = 48;  // LED RGB on-board del DevKitC-1 S3
constexpr uint8_t BOOT_BTN_PIN = 0;   // botón BOOT (strapping, válido como entrada)

// SIM7600 — UART1
constexpr uint8_t SIM_RX_PIN  = 17;
constexpr uint8_t SIM_TX_PIN  = 18;
constexpr uint8_t SIM_PWR_PIN = 4;

// GPS NMEA — UART2
constexpr uint8_t GPS_RX_PIN = 16;
constexpr uint8_t GPS_TX_PIN = 15;

// Ultrasónico HC-SR04 / JSN-SR04T
constexpr uint8_t TRIG_PIN = 5;
constexpr uint8_t ECHO_PIN = 6;

// Sensores analógicos (ADC1 en el S3 = GPIO1..10)
constexpr uint8_t PRESSURE_PIN = 1;  // ADC1_CH0
constexpr uint8_t TEMP_PIN     = 2;  // ADC1_CH1

} // namespace glp::board
