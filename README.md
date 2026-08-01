# LPG IoT Measurement System — ESP32-S3

[![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework](https://img.shields.io/badge/framework-Arduino-teal)](https://github.com/espressif/arduino-esp32)
[![MQTT 5.0](https://img.shields.io/badge/MQTT-5.0-orange)](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
[![Build](https://img.shields.io/badge/PlatformIO-build%20passing-brightgreen)](#installation--build)

---

Firmware for **remote measurement of Liquefied Petroleum Gas (LPG)** in horizontal cylindrical tanks. It estimates the liquid level with an ultrasonic sensor, cross-checks it against pressure and temperature using a joint **2D Extended Kalman Filter (EKF)**, calculates the temperature-compensated volume, mass, and billing-standard gallons (liters corrected to 15 °C), and publishes telemetry over **MQTT 5.0** across **WiFi or 4G cellular (SIM7600)**. It includes **secure OTA with HMAC-SHA256 verification**, **GPS geolocation**, and multi-transport **field provisioning (BLE, WiFi captive portal, and SmartConfig)**. Built on a four-layer **Clean Architecture** (C++17), decoupling business logic from hardware, making it unit-testable on host PCs.

## Table of Contents

- [Key Features](#key-features)
- [Hardware](#hardware)
- [Architecture](#architecture)
- [How the Firmware Boots](#how-the-firmware-boots)
- [Step 1 — Provisioning](#step-1--provisioning)
- [Step 2 — Dual Connectivity (WiFi + Cellular)](#step-2--dual-connectivity-wifi--cellular)
- [Step 3 — Measurement (Sensors → EKF → Volume)](#step-3--measurement-sensors--ekf--volume)
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

### Measurement & Fusion
- **Joint 2D Extended Kalman Filter (EKF)**: Fuses level measurements with a joint state vector $x = [h, T]^T$ (liquid level in mm, temperature in °C). Pressure enters the filter as a second temperature observation via the inverse Antoine equation, sharpening the temperature estimate to reduce variance in density, volume, and mass. Joseph-form covariance updates ensure numerical stability.
- **LPG Thermodynamics (GPA/NIST)**: Saturated liquid density is dynamically calculated via linear interpolation of Propane and Butane density tables based on a remote-configurable propane fraction (`propaneFraction` $\in [0.0, 1.0]$).
- **Billing Correction (VCF)**: Computes physical volume, total mass ($m = \rho(T) \cdot V$), and a standardized volume corrected to 15 °C (`liters_15c`) for billing accuracy.
- **Acoustic Model**: Speed of sound is estimated in the LPG *vapor* (not air) taking real-gas compressibility ($Z \approx 0.8$) and composition into account. Supports a physical reference reflector for automatic speed-of-sound calibration and drift correction.
- **Tilt Compensation**: Corrects ultrasonic level for tank pitch (axial sensor position offset de-bias) and integrates volume across the inclined cylinder segment via 24-point numerical quadrature.
- **Health Diagnostics**: Computes per-sensor health scores (0-100), detects pressure/temperature consistency anomalies against the Antoine vapor pressure curve, and flags physical device anomalies.
- **Autonomy Prediction**: Estimates consumption rate (L/day) using an Exponential Moving Average (EMA, $\alpha=0.2$) on volume reductions, and computes remaining operational days.

### Connectivity
- **Dual Uplink**: Smart dual connectivity with WiFi as the preferred link and automatic fallback to 4G/LTE cellular (SIM7600).
- **MQTT 5.0**: Native session handling, keep-alive, User Properties, and LWT (Last Will and Testament) offline announcements.
- **GPS Integration**: Provides latitude, longitude, and satellite validity for geolocation.

### Field Operation
- **Provisioning without Reflashing**: Interactive BLE GATT configuration, local captive portal over SoftAP, and ESP-Touch SmartConfig.
- **Secure OTA**: Firmware chunks pushed over MQTT with HMAC-SHA256 signature verification and partition verification.
- **NVS Configuration Map**: Stores calibration, connection, and geometry configurations using a 15-character namespace safety limit.

---

## Hardware

**Board:** ESP32-S3-WROOM-1-**N16R8** — 16 MB flash, 8 MB octal PSRAM, 3.3 V logic.

Physical pins are mapped in a centralized location:
[BoardConfig.h](src/Infrastructure/Config/BoardConfig.h).

| Component | Model | Interface | Pins (S3) | Details / Configuration |
|---|---|---|---|---|
| **MCU Board** | ESP32-S3 DevKitC-1 | - | - | 16 MB Flash, 8 MB Octal PSRAM |
| **4G/LTE module** | SIM7600 | UART1 | RX=17, TX=18, PWR=4 | Cell link |
| **GPS** | generic NMEA | UART2 | RX=16, TX=15 | Geolocation |
| **Ultrasonic** | HC-SR04 / JSN-SR04T | Digital | TRIG=5, ECHO=6 | Level sensor time of flight |
| **External ADC** | ADS1115 (16-bit) | I2C (0x48) | SDA=S3 Def, SCL=S3 Def | High precision analog converter |
| **Pressure Sensor** | Analog 0.5V-4.5V | ADC (ADS1115) | Channel A0 | Digitalized over external ADC |
| **Temperature Sensor**| Pt1000 RTD | ADC (ADS1115) | Channel A1 | Divisor with reference resistor |
| **Status LED** | Onboard WS2812 / GPIO | Digital | GPIO 48 | RGB / Status signaling |
| **Boot Button** | BOOT button | Digital | GPIO 0 (pull-up) | Forced provisioning trigger |

> [!WARNING]
> The ESP32-S3 operates on 3.3 V. 5 V signals from sensors (e.g., JSN-SR04T ECHO or 5 V analog pressure sensor outputs) MUST pass through a voltage divider or level shifter before reaching the ESP32-S3 pins or the ADC inputs.

---

## Architecture

Built on a four-layer **Clean Architecture**, enforcing the dependency rule where dependencies point strictly inward:

```text
Presentation ──► Application ──► Domain ◄── Infrastructure
        └───────────────► Infrastructure ──────────┘
```

| Layer | Responsibility | Directory | Dependencies |
|---|---|---|---|
| **Domain** | Entities, interfaces, EKF, sensor fusion, thermodynamics, and acoustic models. Pure C++17, no framework code. | [Domain](src/Domain) | None |
| **Application** | Services (Config, Connectivity, Telemetry), DTOs, and the system state controller. Decoupled orchestration. | [Application](src/Application) | Domain |
| **Infrastructure** | Concrete implementations (WiFi, Cellular, MQTT, NVS, ADS1115 ADC, GPS, Ble/SoftAP provisioning, OTA). | [Infrastructure](src/Infrastructure) | Domain |
| **Presentation** | Setup and main loop execution. Serves as the composition root using manual dependency injection. | [Presentation](src/Presentation) | Domain, Application, Infrastructure |

> Namespaces: `glp::domain`, `glp::app`, `glp::infra`, `glp::board`.

---

## How the Firmware Boots

`Presentation/main.cpp` instantiates all infrastructure elements, injects them into application services, and passes control to `FirmwareController`:

```cpp
void setup() {
    // 1. Initialize peripherals (NVS, raw sensors, GPS)
    // 2. Load persistent configurations from NVS
    // 3. Build dependency graph (DI) and instantiate application services
    // 4. Decide initial state:
    controller.setup(shouldForceProvisioning()); // Holding BOOT pin >5s forces provisioning
}

void loop() {
    controller.tick(); // Runs one iteration of the active state machine
}
```

The state machine lifecycle:

```text
[First boot / BOOT >5s held]
        │
        ▼
   PROVISIONING ──(config saved)──► reboot
        │  LED: Fast blink
        ▼
   CONNECTING ──(link + MQTT OK)──► OPERATIONAL
        │  LED: Slow blink            │  LED: Solid
        └──────────────◄─(link lost)──┘
```

---

## Step 1 — Provisioning

On first boot (unprovisioned flag in NVS) or if the **BOOT button** is held for **> 5 seconds** at startup, the device enters the `PROVISIONING` state. It runs three provisioning portals **sequentially** with timeouts until valid configuration data is saved:

```text
PROVISIONING
 ├─ 1) BLE Portal (30s)        NimBLE Service. Advertises as "GLP-<device_id>"
 ├─ 2) SoftAP Portal (5 min)   Access Point "GLP-CONFIG-<device_id>", Gateway 192.168.4.1
 └─ 3) SmartConfig (2 min)     Native ESP-Touch v1 (WiFi credentials only)
        │
        └─ config received → NVS write → auto-reboot
```

### BLE GATT Specification
The BLE provisioning portal starts a GATT Server with the Service UUID: `12345678-1234-1234-1234-123456789abc`. 
Clients can write configurations to the following characteristic UUIDs:

| Characteristic UUID | Parameter | Type / Description |
|---|---|---|
| `aaaaaaaa-bbbb-cccc-dddd-000000000001` | `apn` | Cellular APN string |
| `aaaaaaaa-bbbb-cccc-dddd-000000000002` | `apn_user` | Cellular APN username |
| `aaaaaaaa-bbbb-cccc-dddd-000000000003` | `apn_pass` | Cellular APN password |
| `aaaaaaaa-bbbb-cccc-dddd-000000000004` | `mqtt_broker` | MQTT broker host string |
| `aaaaaaaa-bbbb-cccc-dddd-000000000005` | `mqtt_port` | MQTT port (sent as string) |
| `aaaaaaaa-bbbb-cccc-dddd-000000000006` | `mqtt_user` | MQTT username |
| `aaaaaaaa-bbbb-cccc-dddd-000000000007` | `mqtt_pass` | MQTT password |
| `aaaaaaaa-bbbb-cccc-dddd-000000000008` | `device_id` | Unique device identifier |
| `aaaaaaaa-bbbb-cccc-dddd-000000000009` | `tank_radius_cm`| Tank inner radius (cm) |
| `aaaaaaaa-bbbb-cccc-dddd-000000000010` | `tank_length_cm`| Tank inner length (cm) |
| `aaaaaaaa-bbbb-cccc-dddd-000000000011` | `ota_hmac_key` | HMAC security key |
| `aaaaaaaa-bbbb-cccc-dddd-000000000012` | `wifi_ssid` | WiFi SSID string |
| `aaaaaaaa-bbbb-cccc-dddd-000000000013` | `wifi_pass` | WiFi password string |
| `aaaaaaaa-bbbb-cccc-dddd-000000000099` | `save` | Write "SAVE" or any value to persist config & reboot |

---

## Step 2 — Dual Connectivity (WiFi + Cellular)

`ConnectivityService` manages network link lifecycle and socket routing:

- **Priority Uplink (WiFi)**: If `wifi_ssid` is configured, it attempts WiFi connection (`WifiLink`).
- **Backup Uplink (4G SIM7600)**: If WiFi fails or is not configured, it initializes `CellularLink` via TinyGSM.
- **Bridge Socket**: `NetworkClient::getActiveNetworkClient()` binds the MQTT socket to either the native `WiFiClient` or `TinyGsmClientBridge`.
- **Time Synchronization**: On connection, registers with NTPServer to fetch UTC time for telemetric timestamping.

---

## Step 3 — Measurement (Sensors → EKF → Volume)

The measurement pipeline runs every **30 seconds**:

```text
Raw Inputs                     Signal Processing                     Outputs & States
─────────────────              ─────────────────                     ─────────────────
Ultrasonic tFlight ──► [Acoustic Model] ──► Level d ─────────────────┐
Pt1000 Temp V ───────► [Calibration]   ──► Temp Pt1000 ──────────────┼──► [State EKF (2D)]
Pressure V ──────────► [Calibration]   ──► Pressure P ──► [Antoine⁻¹] ┘    ├─ Level h_est ──► [Volume Tilted] ──► volume, gallons, %
                                                                             └─ Temp t_est  ──► [LpgThermo] ─────► density, mass, liters_15c

* Note: Pressure also passes through a separate 1D Kalman Filter for reporting and Antoine calculation.
```

### 1. Extended Kalman Filter (EKF)
Fuses ultrasonic distance, Pt1000 RTD resistance, and pressure via a 2D state vector $x = [h, T]^T$.
- **Antoine Inverse**: Translates measured vapor pressure $P$ into a secondary temperature reading:
  $$\log_{10}(P) = A - \frac{B}{C + T_K}$$
- **Sequential Scalar Update**: Applies corrections sequentially (since measurements are uncorrelated/diagonal noise matrices), preventing matrix inversions.
- **Joseph-Form Covariance**: Updates state uncertainty covariance matrix $P$ to guarantee numerical stability:
  $$P_k = (I - K_k H_k) P_{k|k-1} (I - K_k H_k)^T + K_k R_k K_k^T$$

#### EKF Calibration and Noise Covariances:
- **Process Noise (Q)**: Level $q_h = 0.005\text{ mm}^2$, Temperature $q_T = 0.01\text{ }^\circ\text{C}^2$
- **Measurement Noise (R)**: Ultrasonic $r_{US} = 2.5\text{ mm}^2$, Pt1000 $r_{Pt1000} = 0.25\text{ }^\circ\text{C}^2$, Pressure-Derived Temp $r_{Antoine} = 1.0\text{ }^\circ\text{C}^2$
- **Separate Pressure Filter (1D Kalman)**: Process noise $q_P = 0.01\text{ bar}^2$, Measurement noise $r_P = 0.5\text{ bar}^2$

### 2. Acoustic Model (Sound Speed in Vapor)
The speed of sound $c$ in LPG vapor varies heavily with temperature and gas mixture:
- **Primary Source (Reflector)**: If a physical reference reflector is mounted at distance $D_{ref}$:
  $$c = \frac{2 \cdot D_{ref}}{t_{ref}} \text{ (m/s)}$$
- **Fallback Source (Thermodynamics)**: Based on real-gas speed of sound model:
  $$c = \sqrt{\frac{\gamma \cdot Z \cdot R \cdot T}{M}}$$
  Where compressibility $Z \approx 0.8$, universal gas constant $R = 8.314\text{ J/(mol}\cdot\text{K)}$, and heat capacity ratio $\gamma$ and molar mass $M$ are calculated by composition weighting via `propaneFraction`.
- **Distance to Liquid**: $d = \frac{c \cdot t_{flight}}{2 \cdot 1000}\text{ (mm)}$
- **Calculated Level**: $h = H - d - mountOffset$, clamped to $[0, 2R]$.

### 3. Tilt Correction (Tilted Tank Volume)
- **Axial Sensor Offset**: If the sensor is mounted off-center by $x_{sensor}$:
  $$h_{center} = h_{measured} - x_{sensor} \cdot \tan(\theta_{pitch})$$
- **Volume Integration**: Volume is not calculated as a flat segment $A(h) \cdot L$. Instead, it numerical-integrates the segment area along the pitch angle using a 24-node midpoint quadrature:
  $$V_{Liters} = \sum_{i=1}^{24} \frac{A(h(x)) \cdot dx}{10^6}$$
  Where local height $h(x) = \text{clamp}(h_{center} + x \cdot \tan(\theta_{pitch}), 0, 2R)$, and $dx = L / 24$.

### 4. Thermodynamics, Mass, and Autonomy
- **Density ($\rho$)**: Linearly interpolated from saturated liquid density tables for Propane and Butane at estimated EKF temperature.
- **Custody Transfer ($V_{15}$)**: Computes standardized volume corrected to 15 °C for billing:
  $$V_{15} = \frac{m}{\rho(15)}$$
  Where mass $m = \rho(T) \cdot V_{measured}$.
- **Diagnostics**:
  - **Health**: Evaluation metrics for sensors:
    - Ultrasonic: 0 if invalid; 60 if gas-speed model and reflector disagree.
    - Temperature: 0 if invalid; 50 if outside physical range $[-40, 60]^\circ\text{C}$.
    - Pressure: $100 - |P_{measured} - P_{Antoine}(T)| \cdot 15$, clamped to $[0, 100]$.
  - **Consumption**: Average consumption rate (L/day) using an EMA ($\alpha = 0.2$) on volume reductions.
  - **Autonomy**: $\text{Remaining Days} = \frac{V_{measured}}{\text{ConsumptionRate (L/day)}}$.

---

## Step 4 — MQTT 5.0 Telemetry

Every 30 seconds, the device publishes telemetric data to `glp/telemetria/{device_id}`. Telemetry is suppressed during OTA updates.

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
    "sensors_raw": {
      "level_mm": 326.3,
      "pressure_bar": 6.24,
      "temp_celsius": 28.1
    },
    "sensors_filtered": {
      "level_mm": 325.8,
      "pressure_bar": 6.21,
      "temp_celsius": 27.9,
      "kalman_uncertainty_mm": 0.07
    },
    "volume": {
      "liters": 145.3,
      "gallons": 38.4,
      "percentage": 72.1,
      "density_kg_l": 0.563,
      "mass_kg": 81.8,
      "liters_15c": 143.9,
      "anomaly": false
    },
    "tank": {
      "radius_cm": 45.0,
      "length_cm": 120.0
    },
    "gps": {
      "lat": 18.4861,
      "lng": -69.9312,
      "valid": true
    },
    "rssi": -78,
    "free_heap": 210000,
    "uptime": 3600000,
    "health": {
      "ultrasonic": 100.0,
      "pressure": 95.5,
      "temperature": 100.0
    },
    "vapor_check": {
      "p_measured": 6.21,
      "p_antoine": 6.35,
      "residual": -0.14
    },
    "consumption": {
      "l_per_day": 12.4,
      "remaining_days": 11.7
    },
    "acoustic": {
      "c_used_ms": 226.5,
      "source": "gas_model",
      "healthy": true
    }
  }
}
```

---

## Step 5 — Remote Configuration

With the device online, it listens to the configuration topic `glp/config/{device_id}`. Incoming JSON payloads are parsed dynamically and values are applied immediately to NVS. When geometry parameters change, the EKF filter is reset to converge with the new parameters.

Supported remote configuration fields:

```json
{
  "tank_radius_cm": 45.0,
  "tank_length_cm": 120.0,
  "mqtt_broker": "broker.hivemq.com",
  "mqtt_port": 1883,
  "ota_hmac_key": "s3cr3t-k3y",
  "tank_height_cm": 95.0,
  "mount_offset_cm": 5.0,
  "press_slope": 10.0,
  "press_offset": -0.5,
  "temp_r_ref": 1000.0,
  "temp_v_exc": 3.30,
  "temp_offset_c": 0.0,
  "propane_fraction": 0.7,
  "sensor_axial_cm": 10.0,
  "ref_distance_cm": 0.0
}
```

---

## Step 6 — OTA Updates

Firmware updates are sent asynchronously via MQTT using the `MQTTOTAv5` library:
- **Transport**: Base64 encoded chunks.
- **Verification**: SHA-256 and HMAC-SHA256 signature verification.
- **Failsafe**: If execution fails after an update, the ESP-IDF OTA bootloader performs an automatic rollback to the previous partition.
- **State Blocking**: Telemetry publishing is suppressed during active updates.

---

## MQTT Topics

| Topic | Direction | Content |
|---|---|---|
| `glp/telemetria/{device_id}` | Device → Broker | Telemetry payload JSON |
| `glp/status/{device_id}` | Device → Broker | Will Message: `{"status":"offline","device_id":"..."}` |
| `glp/config/{device_id}` | Broker → Device | Live configuration adjustments JSON |
| `ota/{device_id}` | Broker → Device | Firmware in Base64 chunks |
| `ota/{device_id}/progress` | Device → Broker | OTA progress update percent |
| `ota/{device_id}/success` | Device → Broker | OTA successful installation report |
| `ota/{device_id}/error` | Device → Broker | OTA failure reason payload |

---

## Persistence (NVS)

The persistent storage parameters are managed by the `Preferences` API under the `glp-config` namespace. Due to the 15-character key limit enforced by NVS in ESP-IDF/Arduino, names are mapped as follows:

| NVS Key | JSON Payload Key | C++ Domain Struct Field | Type | Default Value |
|---|---|---|---|---|
| `apn` | `apn` | `apn` | String | `"internet.claro.com.do"` |
| `apn_user` | `apn_user` | `apnUser` | String | `""` |
| `apn_pass` | `apn_pass` | `apnPass` | String | `""` |
| `mqtt_broker` | `mqtt_broker` | `mqttBroker` | String | `"broker.hivemq.com"` |
| `mqtt_port` | `mqtt_port` | `mqttPort` | Int | `1883` |
| `mqtt_user` | `mqtt_user` | `mqttUser` | String | `""` |
| `mqtt_pass` | `mqtt_pass` | `mqttPass` | String | `""` |
| `wifi_ssid` | `wifi_ssid` | `wifiSsid` | String | `""` |
| `wifi_pass` | `wifi_pass` | `wifiPass` | String | `""` |
| `device_id` | `device_id` | `deviceId` | String | `"GLP-001"` |
| `radius` | `tank_radius_cm` | `tankRadiusCm` | Float | `45.0` |
| `length` | `tank_length_cm` | `tankLengthCm` | Float | `120.0` |
| `height` | `tank_height_cm` | `tankHeightCm` | Float | `0.0` |
| `mount_off` | `mount_offset_cm`| `mountOffsetCm` | Float | `0.0` |
| `p_slope` | `press_slope` | `calibration.pressSlope` | Float | `0.0` |
| `p_offset` | `press_offset` | `calibration.pressOffset`| Float | `0.0` |
| `t_rref` | `temp_r_ref` | `calibration.tempRRef` | Float | `1000.0` |
| `t_vexc` | `temp_v_exc` | `calibration.tempVExc` | Float | `3.30` |
| `t_offc` | `temp_offset_c` | `calibration.tempOffsetC`| Float | `0.0` |
| `propane_fr` | `propane_fraction`| `propaneFraction` | Float | `1.0` |
| `sensor_ax` | `sensor_axial_cm` | `sensorAxialCm` | Float | `0.0` |
| `ref_dist` | `ref_distance_cm` | `refDistanceCm` | Float | `0.0` |
| `ota_hmac` | `ota_hmac_key` | `otaHmacKey` | String | `""` |
| `provisioned` | - | `provisioned` | Bool | `false` |

---

## LED Indicators

Peripherals are signaled via `GpioIndicator` mapping active states to specific digital pulse cycles:

| Pulse Pattern | System State | Trigger Condition | Code Pattern |
|---|---|---|---|
| **Fast Blink** | `PROVISIONING` | Provisioning portal active | `blink(100, 100, 3)` (100ms ON / 100ms OFF, 3 cycles) |
| **Slow Blink** | `CONNECTING` | Networking or MQTT handshake | `blink(500, 500, 1)` (500ms ON / 500ms OFF, 1 cycle) |
| **Solid ON** | `OPERATIONAL` | Normal operation (connected) | `digitalWrite(pin_, HIGH)` (GPIO high) |
| **Very Fast Blink**| `Anomaly` | Sensor discrepancy / invalid readings | `blink(50, 50, 6)` (50ms ON / 50ms OFF, 6 cycles) |
| **Long Double Flash**| `Error` | Hardware or connection failures | `blink(1000, 200, 2)` (1000ms ON / 200ms OFF, 2 cycles) |

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

The Domain and Application layers are pure C++17 and can be tested locally on a PC with **CMake + GoogleTest** (no hardware needed):

```bash
cmake -S . -B build -DHOST_BUILD=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Suites: `test_sensor_fusion`, `test_config_service`, `test_connectivity_service`,
`test_telemetry_service`, `test_firmware_controller`, `test_acoustic_model`,
`test_calibrating_sensor_reader`, `test_consumption_estimator`, `test_lpg_thermo`,
`test_sensor_calibrator`, `test_sensor_health`, `test_state_ekf`, `test_tank_geometry`,
`test_tilt_correction`.

---

## Project Structure

```text
GLP_Presure/
├── platformio.ini            # S3 board, dependencies, partitions config
├── partitions.csv            # 16 MB table (app0/app1 = 4 MB, OTA)
├── CMakeLists.txt            # Host build configuration (glp_core + tests)
├── README.md                 # Project documentation (this file)
├── src/
│   ├── Domain/               # Entities, interfaces, EKF/thermo logic (pure C++17)
│   ├── Application/          # Orchestration services, DTOs, controllers
│   ├── Infrastructure/       # Concrete HW drivers (NVS, BLE, WebServer, cellular, ADC)
│   └── Presentation/main.cpp # Setup & loop (Composition root / DI)
└── tests/                    # Host GoogleTest suite & unit test doubles
```

---

## License

Licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

## Contact

Author: **Jorge Gaspar Beltre Rivera**  
Project: **LPG IoT Measurement System — ESP32-S3 firmware**

<p align="center">
  <a href="https://www.linkedin.com/in/jorge-gaspar-beltre-rivera/" target="_blank"><img src="https://user-images.githubusercontent.com/74038190/235294012-0a55e343-37ad-4b0f-924f-c8431d9d2483.gif" alt="LinkedIn" width="100"></a>
  <a href="https://github.com/JorgeGBeltre" target="_blank"><img src="https://user-images.githubusercontent.com/74038190/212257468-1e9a91f1-b626-4baa-b15d-5c385dfa7ed2.gif" alt="GitHub" width="100"></a>
  <a href="mailto:Jorgegaspar3021@gmail.com"><img src="https://user-images.githubusercontent.com/74038190/216122065-2f028bae-25d6-4a3c-bc9f-175394ed5011.png" alt="E-Mail" width="100"></a>

</p>

## Support

This project is developed independently. Even a small contribution helps me dedicate more time to development, testing, and releasing new features.


 <p align="center">
  <a href="https://www.paypal.com/donate/?hosted_button_id=2VLA8BWT967LU">
    <img src="https://www.paypalobjects.com/webstatic/icon/pp258.png"
         alt="Donate with PayPal"
         height="60">
  </a>
</p>
