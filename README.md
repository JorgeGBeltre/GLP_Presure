# LPG IoT Measurement System — ESP32-S3

[![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework](https://img.shields.io/badge/framework-Arduino-teal)](https://github.com/espressif/arduino-esp32)
[![MQTT 5.0](https://img.shields.io/badge/MQTT-5.0-orange)](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
[![Build](https://img.shields.io/badge/PlatformIO-build%20passing-brightgreen)](#installation--build)

---

Firmware for **remote measurement of Liquefied Petroleum Gas (LPG)** in horizontal
cylindrical tanks. It measures the liquid level with an ultrasonic sensor,
cross-checks it against pressure and temperature, filters everything with a
**Kalman filter**, computes the temperature-compensated volume and gallons, and
publishes telemetry over **MQTT 5.0** across **WiFi or 4G cellular** with a
well-structured JSON. It includes **secure OTA**, **GPS**, and **field
provisioning** (BLE, WiFi captive portal, and SmartConfig). Built on a four-layer
**Clean Architecture**.

## Table of Contents

- [Key Features](#key-features)
- [Hardware](#hardware)
- [Architecture](#architecture)
- [How the Firmware Boots](#how-the-firmware-boots)
- [Step 1 — Provisioning](#step-1--provisioning)
- [Step 2 — Dual Connectivity (WiFi + Cellular)](#step-2--dual-connectivity-wifi--cellular)
- [Step 3 — Measurement (Sensors → Kalman → Volume)](#step-3--measurement-sensors--kalman--volume)
- [Step 4 — MQTT 5.0 Telemetry](#step-4--mqtt-50-telemetry)
- [Step 5 — Remote Configuration](#step-5--remote-configuration)
- [Step 6 — OTA Updates](#step-6--ota-updates)
- [MQTT Topics](#mqtt-topics)
- [Persistence (NVS)](#persistence-nvs)
- [LED Indicators](#led-indicators)
- [Installation & Build](#installation--build)
- [Project Structure](#project-structure)
- [License](#license)
- [Contact](#contact)
- [Support](#support)

---

## Key Features

### Measurement
- **Sensor fusion** — ultrasonic (level) + pressure + temperature.
- **Scalar Kalman filter** per sensor: removes noise without adding latency.
- **Sensor calibration** — Pt1000 (IEC 60751) temperature and 2-point linear pressure,
  configured at provisioning; ultrasonic level as `h = H − d` with dead-zone offset.
- **Invalid-reading safety** — a timed-out/out-of-range channel makes the Kalman filter
  predict-only (holds the estimate, grows uncertainty) instead of ingesting garbage.
- **Dynamic volume** — circular-segment formula for a horizontal cylindrical tank,
  with configurable dimensions.
- **Mass & density** — interpolated propane/butane saturated-liquid density table
  (remote-configurable propane fraction), LPG **mass** `m = ρ(T)·V`, and a separate
  volume-corrected-to-15 °C output (`liters_15c`) for custody/billing.
- **Anomaly detection** — high level with low pressure (possible leak/faulty sensor).

### Connectivity
- **Dual uplink** — WiFi (preferred) with **fallback to 4G/LTE cellular (SIM7600)**.
- **MQTT 5.0** — Will Message, auto-reconnect, session properties.
- **GPS** — tank geolocation and timestamp.

### Field operation
- **Provisioning without reflashing** — BLE, WiFi captive portal, and SmartConfig.
- **Secure OTA** — firmware over MQTT with **HMAC-SHA256** verification and rollback.
- **Structured JSON** — common `{Device, Timestamp, Details}` envelope.

### Engineering
- **Clean Architecture** — pure Domain/Application (C++17, no Arduino), unit-testable
  on a PC with GoogleTest.
- **Centralized pin map** in a single header.

---

## Hardware

**Board:** ESP32-S3-WROOM-1-**N16R8** — 16 MB flash, 8 MB octal PSRAM, 3.3 V logic.

Every pin is defined in a single place:
`src/Infrastructure/Config/BoardConfig.h`.

| Component | Model | Interface | Pins (S3) |
|---|---|---|---|
| 4G/LTE module | SIM7600 | UART1 | RX=17, TX=18, PWR=4 |
| GPS | generic NMEA | UART2 | RX=16, TX=15 |
| Ultrasonic | HC-SR04 / JSN-SR04T | Digital | TRIG=5, ECHO=6 |
| Pressure | analog sensor | ADC1 | GPIO 1 |
| Temperature | analog sensor | ADC1 | GPIO 2 |
| Status LED | GPIO | GPIO | 48 |
| Provisioning button | BOOT | GPIO | 0 (pull-up) |

> **Voltage:** the S3 is 3.3 V. With 5 V sensors, use a voltage divider or level
> shifter on `ECHO` and on the ADC inputs.

**Power:** 12/24 V battery → 3 A+ buck (LM2596 / MP1584) → 3.3/5 V. The SIM7600
draws > 2 A peaks during transmission — size the supply accordingly.

---

## Architecture

Four layers, with the dependency rule always pointing inward:

```text
Presentation ──► Application ──► Domain ◄── Infrastructure
        └───────────────► Infrastructure ──────────┘
```

| Layer | Responsibility | Depends on |
|---|---|---|
| **Domain** (`src/Domain`) | Entities, algorithms (Kalman, fusion), and interfaces. Pure C++17, zero Arduino. | Nothing |
| **Application** (`src/Application`) | Orchestration services and the state machine, over interfaces. | Domain |
| **Infrastructure** (`src/Infrastructure`) | Concrete implementations (Arduino/ESP-IDF/libraries) of the interfaces. | Domain |
| **Presentation** (`src/Presentation`) | `main.cpp`: composition and dependency injection. | All |

> Namespaces: `glp::domain`, `glp::app`, `glp::infra`, `glp::board`.

---

## How the Firmware Boots

`Presentation/main.cpp` is the composition root: it instantiates Infrastructure,
injects it into Application, and delegates the lifecycle to `FirmwareController`.

```cpp
void setup() {
    // 1. Initialize peripherals (NVS, sensors, GPS) and load the config
    // 2. Build the dependency graph (DI) and the services
    // 3. Register SNTP and the remote-config handler
    // 4. Decide the initial state (provisioned -> CONNECTING; otherwise -> PROVISIONING)
    controller.setup(shouldForceProvisioning());   // holding BOOT >5s forces provisioning
}

void loop() {
    controller.tick();   // one iteration of the state machine
}
```

The state machine:

```text
[First boot / BOOT >5s held]
        │
        ▼
   PROVISIONING ──(config saved)──► reboot
        │  LED: fast blink
        ▼
   CONNECTING ──(link + MQTT OK)──► OPERATIONAL
        │  LED: slow blink            │  LED: solid
        └──────────────◄─(link lost)──┘
```

---

## Step 1 — Provisioning

On first boot (empty NVS) or by holding **BOOT > 5 s**, the device enters
`PROVISIONING` and tries three transports **in order** until one delivers the
configuration. BLE and SoftAP deliver the full configuration (including WiFi
credentials); SmartConfig delivers only the WiFi SSID/password.

```text
PROVISIONING
 ├─ 1) BLE (30 s)          NimBLE, advertises "GLP-<device_id>"       → nRF Connect
 ├─ 2) SoftAP (5 min)      AP "GLP-CONFIG-<device_id>", 192.168.4.1   → browser
 └─ 3) SmartConfig (2 min) ESP-Touch v1 (SC_TYPE_ESPTOUCH)            → EspTouch app
        │
        └─ config → NVS → reboot
```

| Transport | Implementation | Delivers |
|---|---|---|
| **BLE** | NimBLE, one GATT characteristic per parameter + a "save" one (`...099`) | Full config + WiFi |
| **SoftAP** | ESPAsyncWebServer + DNSServer (captive portal) | Full config + WiFi |
| **SmartConfig** | native `esp_smartconfig` (`WiFi.beginSmartConfig(SC_TYPE_ESPTOUCH)`) | WiFi credentials only |

**Configurable parameters:** `apn` · `apn_user` · `apn_pass` · `wifi_ssid` ·
`wifi_pass` · `mqtt_broker` · `mqtt_port` · `mqtt_user` · `mqtt_pass` ·
`device_id` · `tank_radius_cm` · `tank_length_cm` · `ota_hmac_key`.

---

## Step 2 — Dual Connectivity (WiFi + Cellular)

After provisioning, `ConnectivityService` brings up the link and then the MQTT
transport on top of it:

```text
connect()
 ├─ wifi_ssid set?  ── yes ─► WiFiLink.connect()     (WiFi STA)
 │                    └ no ──► CellularLink.connect() (SIM7600 / TinyGSM)
 ├─ if WiFi fails ──────────► fall back to cellular
 └─ link is up ────────────► MQTT.connect() over the active link's client
```

- `NetworkClient::getActiveNetworkClient()` hands the MQTT client either a
  `WiFiClient` (TCP over WiFi) or the `TinyGsmClientBridge` (TCP over the modem),
  depending on the active link.
- On connect, SNTP (UTC) is registered for the telemetry timestamp.
- In `OPERATIONAL`, if the active link drops it goes back to `CONNECTING`; if only
  MQTT drops, it retries the reconnection.

---

## Step 3 — Measurement (Sensors → Kalman → Volume)

In `OPERATIONAL`, the measurement pipeline runs every **30 s**:

```text
raw readings              filtering (Kalman)         computation
─────────────────         ─────────────────          ───────────────────────────
ultrasonic  h_raw ─► [Kalman level]     ─► h_est ─► [Cylindrical volume] ─► liters
pressure    p_raw ─► [Kalman pressure]  ─► p_est ─► (cross-validation)   ─► gallons
temperature t_raw ─► [EMA]              ─► t_est ─► LPG density (kg/L)
```

**1. Kalman filter** (per sensor):
```text
P = P + Q                 // predict
K = P / (P + R)           // gain
x = x + K·(z − x)         // correct
P = (1 − K)·P
```

| Sensor | Q | R |
|---|---|---|
| Ultrasonic (level) | 0.005 mm² | 2.5 mm² |
| Pressure | 0.01 bar² | 0.5 bar² |
| Temperature | EMA α = 0.1 | — |

**2. Volume** (circular segment, horizontal cylindrical tank):
```text
θ = 2·arccos((R − h)/R)
A = R²·(θ − sin θ)/2
V_liters  = A·L / 1e6
V_gallons = V_liters · 0.264172
%fill     = V_liters / V_total · 100
```

**3. Density and anomaly:**
```text
LPG_density = 0.58 − 0.00125·(T − 15)   [kg/L]   (minimum 0.45)
anomaly     = (%fill > 5) && (pressure < 1 bar)
```

`R` and `h` are in mm (`R = tank_radius_cm·10`); `L = tank_length_cm·10`.

---

## Step 4 — MQTT 5.0 Telemetry

Every 30 s the device publishes to `glp/telemetria/{device_id}` a JSON with the
common **`{Device, Timestamp, Details}`** envelope: the `Device` + `Timestamp`
header is shared by every message; `Details` carries the LPG measurement. The
header is assembled from the device identity (`IDeviceRepository`), the active link
(`ConnectivityService`), and the clock (`ITimeProvider`).

```json
{
  "Device": {
    "ChipId": "A1B2C3D4E5F6",
    "ChipType": "ESP32-S3",
    "MacAddress": "XX:XX:XX:XX:XX:XX",
    "FirmwareVersion": "1.0.0",
    "DeviceId": "GLP-001",
    "IMEI": "",
    "IMSI": "",
    "status": "connected",
    "TypeConexion": "Wifi",
    "IPAddress": "192.168.1.100"
  },
  "Timestamp": "2026-07-03T10:30:00.123Z",
  "Details": {
    "sensors_raw":      { "level_mm": 326.3, "pressure_bar": 6.24, "temp_celsius": 28.1 },
    "sensors_filtered": { "level_mm": 325.8, "pressure_bar": 6.21, "temp_celsius": 27.9, "kalman_uncertainty_mm": 0.07 },
    "volume": { "liters": 145.3, "gallons": 38.4, "percentage": 72.1, "density_kg_l": 0.563, "anomaly": false },
    "tank":  { "radius_cm": 45.0, "length_cm": 120.0 },
    "gps":   { "lat": 18.4861, "lng": -69.9312, "valid": true },
    "rssi": -78, "free_heap": 210000, "uptime": 3600000
  }
}
```

Publishing is **suppressed** if the transport is disconnected or if an OTA is in
progress.

---

## Step 5 — Remote Configuration

With the device online, its configuration can be updated without entering
provisioning, by publishing to `glp/config/{device_id}`:

```json
{ "tank_radius_cm": 50.0, "tank_length_cm": 140.0, "mqtt_broker": "broker.new.com", "mqtt_port": 8883, "ota_hmac_key": "new-key" }
```

The firmware parses the message (`RemoteConfigParser`), applies the present fields
(`ConfigService`), persists to NVS, and **resets the Kalman filters** so they
re-converge with the new geometry.

---

## Step 6 — OTA Updates

The backend sends the firmware over MQTT and the device applies it without blocking
the loop:

```text
Backend                         Device
  │                                  │
  │── firmware (Base64, chunks) ────►│  MQTTOTAv5 receives and decodes
  │                                  │  verifies SHA-256 + HMAC-SHA256
  │◄── ota/{id}/progress ───────────│  writes the OTA partition
  │◄── ota/{id}/success ────────────│  automatic reboot
  │                                  │  (if boot fails → rollback)
```

- The HMAC key is provisioned as `ota_hmac_key`.
- `IOtaService::isInProgress()` reflects the real state (a flag toggled in the OTA
  callbacks), and telemetry is suppressed while the update is running.

---

## MQTT Topics

| Topic | Direction | Content |
|---|---|---|
| `glp/telemetria/{device_id}` | Device → Broker | Telemetry (`{Device, Timestamp, Details}` envelope) |
| `glp/status/{device_id}` | Device → Broker | Will Message `{"status":"offline"}` |
| `glp/config/{device_id}` | Broker → Device | Configuration update |
| `ota/{device_id}` | Broker → Device | Firmware in Base64 chunks |
| `ota/{device_id}/progress` | Device → Broker | OTA progress |
| `ota/{device_id}/success` · `/error` | Device → Broker | OTA outcome |

---

## Persistence (NVS)

Configuration lives in the `glp-config` namespace (`Preferences` API):

`apn` · `apn_user` · `apn_pass` · `wifi_ssid` · `wifi_pass` · `mqtt_broker` ·
`mqtt_port` · `mqtt_user` · `mqtt_pass` · `device_id` · `radius` · `length` ·
`ota_hmac` · `provisioned`.

---

## LED Indicators

| LED | State |
|---|---|
| Fast blink | PROVISIONING |
| Slow blink | CONNECTING |
| Solid | OPERATIONAL |
| Error blink | Failure / reconnecting |

---

## Installation & Build

Requires [PlatformIO Core](https://platformio.org/).

```bash
pio run                 # build (env: esp32-s3-n16r8)
pio run -t upload       # flash the ESP32-S3
pio device monitor      # serial monitor (115200 baud)
```

Board configuration (`platformio.ini`):

```ini
[env:esp32-s3-n16r8]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.arduino.memory_type = qio_opi   ; QIO flash + octal PSRAM (R8)
board_build.partitions = partitions.csv      ; 16 MB: app0/app1 = 4 MB (OTA)
build_unflags = -std=gnu++11
build_flags   = -std=gnu++17 -DBOARD_HAS_PSRAM -DTINY_GSM_MODEM_SIM7600 -I src
```

**Dependencies** (`lib_deps`): `TinyGSM`, `TinyGPSPlus`,
[`ESP32_MQTTv5`](https://github.com/JorgeGBeltre/ESP32_MQTTv5),
[`MQTTOTAv5`](https://github.com/JorgeGBeltre/MQTTOTAv5), `ArduinoJson`,
`NimBLE-Arduino`, `ESPAsyncWebServer`, `AsyncTCP`.

### Host tests (Domain + Application)

The Domain and Application layers are pure C++17 and are tested on a PC with
**CMake + GoogleTest** (no hardware):

```bash
cmake -S . -B build -DHOST_BUILD=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Suites: `test_sensor_fusion`, `test_config_service`, `test_connectivity_service`,
`test_telemetry_service`, `test_firmware_controller`.

---

## Project Structure

```text
GLP_Presure/
├── platformio.ini            # S3 board, dependencies, partitions
├── partitions.csv            # 16 MB table (app0/app1 = 4 MB, OTA)
├── CMakeLists.txt            # host build (glp_core + tests)
├── README.md                 # this file
├── src/
│   ├── Domain/               # entities + interfaces + Kalman/fusion (pure)
│   ├── Application/          # services + FirmwareController + DTOs (envelope)
│   ├── Infrastructure/       # Config Sensors Gps Net Messaging Ota Provisioning
│   │                         #   System Time Serialization Indicator Logging
│   └── Presentation/main.cpp # composition root (DI)
└── tests/                    # host GoogleTest + doubles (Fakes.h)
```

---

## License

Licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

## Contact

Author: **Jorge Gaspar Beltre Rivera**  
Project: **LPG IoT Measurement System — ESP32-S3 firmware**

 [![GitHub](https://img.shields.io/badge/GitHub-181717?style=for-the-badge&logo=github&logoColor=white)](https://github.com/JorgeGBeltre)
 [![LinkedIn](https://img.shields.io/badge/LinkedIn-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white)](https://www.linkedin.com/in/jorge-gaspar-beltre-rivera/)
 [![Email](https://img.shields.io/badge/Email-EA4335?style=for-the-badge&logo=gmail&logoColor=white)](mailto:Jorgegaspar3021@gmail.com)

---

##  Support

This project is developed independently.

Even a small contribution helps me dedicate more time to development, testing, and releasing new features.

 [![Buy Me a Coffee](https://img.shields.io/badge/Buy_Me_a_Coffee-FFDD00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://www.paypal.com/donate/?hosted_button_id=2VLA8BWT967LU)