// TINY_GSM_MODEM_SIM7600 is defined via build_flags in platformio.ini
#include <TinyGsmClient.h>
#include "sim_module.h"
#include "Infrastructure/Config/nvs_config.h"

#include "Infrastructure/Config/BoardConfig.h"

// Pines centralizados en BoardConfig.h (válidos para el S3).
static constexpr uint8_t SIM_TX_PIN  = glp::board::SIM_TX_PIN;
static constexpr uint8_t SIM_RX_PIN  = glp::board::SIM_RX_PIN;
static constexpr uint8_t SIM_PWR_PIN = glp::board::SIM_PWR_PIN;
#define SIM_BAUD   115200

HardwareSerial simSerial(1);
TinyGsm        modem(simSerial);

// Bridge wraps TinyGsmClient to present WiFiClient interface for ESP32_MQTTv5
TinyGsmClientBridge& getSimClient() {
    static TinyGsmClient    rawClient(modem);
    static TinyGsmClientBridge bridge(rawClient);
    return bridge;
}


static int _rssi = -999;

static void powerOnSIM() {
    pinMode(SIM_PWR_PIN, OUTPUT);
    digitalWrite(SIM_PWR_PIN, HIGH);
    delay(1500);
    digitalWrite(SIM_PWR_PIN, LOW);
    delay(3000);
}

bool initSIM() {
    simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
    Serial.println("[SIM] Initializing SIM7600 (4G/LTE)...");

    powerOnSIM();

    if (!modem.init()) {
        Serial.println("[SIM] Modem init FAILED");
        return false;
    }

    String info = modem.getModemInfo();
    Serial.println("[SIM] Modem: " + info);

    // Backoff reconnect: up to 5 retries with exponential delay
    int  retry   = 0;
    long delayMs = 2000;
    while (!modem.gprsConnect(config.apn.c_str(),
                               config.apn_user.c_str(),
                               config.apn_pass.c_str())) {
        retry++;
        if (retry > 5) {
            Serial.println("[SIM] GPRS connect FAILED after 5 retries");
            return false;
        }
        Serial.printf("[SIM] GPRS attempt %d failed — retrying in %ldms\n", retry, delayMs);
        delay(delayMs);
        delayMs = min(delayMs * 2, 60000L);
    }

    Serial.println("[SIM] GPRS/LTE connected!");
    _rssi = modem.getSignalQuality();
    Serial.printf("[SIM] Signal quality: %d\n", _rssi);
    return true;
}

bool isSimConnected() {
    return modem.isGprsConnected();
}

int getSignalRSSI() {
    _rssi = modem.getSignalQuality();
    return _rssi;
}

std::string getSimLocalIp() {
    if (!modem.isGprsConnected()) return "0.0.0.0";
    return std::string(modem.localIP().toString().c_str());
}
