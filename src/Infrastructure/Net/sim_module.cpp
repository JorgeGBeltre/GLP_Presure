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

namespace {
struct CarrierApnEntry {
    const char* mccMncPrefix;
    const char* apn;
};

constexpr CarrierApnEntry kCarrierApnTable[] = {
    {"37001", "internet.orange.com.do"},
    {"37002", "internet.claro.com.do"},
    {"37003", "avircom.net"},
    {"37004", "data.viva.com.do"},
    {"334020", "internet.itelcel.com"},
    {"33403", "internet.movistar.mx"},
    {"732101", "internet.comcel.com.co"},
    {"732103", "web.colombiamovil.com.co"},
    {"732123", "internet.movistar.com.co"},
    {"71606", "movistar.pe"},
    {"71610", "claro.pe"},
    {"73001", "bam.entelpcs.cl"},
    {"73002", "wap.tmovil.cl"},
    {"72207", "wap.gprs.unifon.com.ar"},
    {"722310", "igprs.claro.com.ar"},
    {"310260", "fast.t-mobile.com"},
    {"310410", "broadband"},
    {"21401", "airtelnet.es"},
    {"21407", "movistar.es"},
};

String autoResolveApnFromImsi(const String& imsi) {
    for (size_t i = 0; i < sizeof(kCarrierApnTable) / sizeof(kCarrierApnTable[0]); ++i) {
        if (imsi.startsWith(kCarrierApnTable[i].mccMncPrefix)) {
            return String(kCarrierApnTable[i].apn);
        }
    }
    return "";
}
} // namespace

// Bridge wraps TinyGsmClient to present WiFiClient interface for ESP32_MQTTv5
TinyGsmClientBridge& getSimClient() {
    static TinyGsmClient    rawClient(modem);
    static TinyGsmClientBridge bridge(rawClient);
    return bridge;
}

static int _rssi = -999;
static int _lastValidRssi = 0;
static bool _jammingDetected = false;
static std::string _resolvedApn;

static void powerOnSIM() {
    pinMode(SIM_PWR_PIN, OUTPUT);
    digitalWrite(SIM_PWR_PIN, HIGH);
    delay(1500);
    digitalWrite(SIM_PWR_PIN, LOW);
    delay(3000);
}

static String queryModemLine(const char* cmd, unsigned long timeoutMs) {
    while (simSerial.available()) simSerial.read();
    simSerial.println(cmd);
    unsigned long start = millis();
    String resp;
    while (millis() - start < timeoutMs) {
        while (simSerial.available()) {
            resp += static_cast<char>(simSerial.read());
        }
        if (resp.indexOf("OK") >= 0 || resp.indexOf("ERROR") >= 0) break;
        delay(10);
    }
    return resp;
}

void probeSimProvisioningState() {
    simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
    delay(80);
    if (!modem.testAT(600)) {
        setCellularProvisioningSnapshot(false, false, false, "");
        return;
    }
    const bool simReady = (modem.getSimStatus() == SIM_READY);
    bool autoApn = false;
    bool gprsConn = modem.isGprsConnected();
    String detectedApn;

    if (simReady) {
        String imsi = modem.getIMSI();
        if (imsi.length() >= 5) {
            detectedApn = autoResolveApnFromImsi(imsi);
            if (!detectedApn.isEmpty()) {
                autoApn = true;
            }
        }
        if (detectedApn.isEmpty()) {
            String cgdcont = queryModemLine("AT+CGDCONT?", 800);
            int tag = cgdcont.indexOf("+CGDCONT:");
            if (tag >= 0) {
                int qCount = 0;
                int qStart = -1;
                for (int i = tag; i < (int)cgdcont.length(); ++i) {
                    if (cgdcont[i] == '"') {
                        ++qCount;
                        if (qCount == 3) qStart = i + 1;
                        else if (qCount == 4 && qStart > 0) {
                            String cand = cgdcont.substring(qStart, i);
                            cand.trim();
                            if (cand.length() > 0) {
                                detectedApn = cand;
                                autoApn = true;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    if (!detectedApn.isEmpty()) {
        _resolvedApn = detectedApn.c_str();
    }
    setCellularProvisioningSnapshot(simReady, autoApn, gprsConn, _resolvedApn.c_str());
    Serial.printf("[SIM] Sondeo celular GLP: SIM=%d, autoAPN=%d (%s), LTE=%d\n",
                  simReady ? 1 : 0, autoApn ? 1 : 0, _resolvedApn.c_str(), gprsConn ? 1 : 0);
}

bool initSIM() {
    simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
    Serial.println("[SIM] Initializing SIM7600 (4G/LTE)...");

    if (!modem.testAT(600)) {
        powerOnSIM();
    }

    if (!modem.init()) {
        Serial.println("[SIM] Modem init FAILED");
        setCellularProvisioningSnapshot(false, false, false, "");
        return false;
    }

    String info = modem.getModemInfo();
    Serial.println("[SIM] Modem: " + info);

    String imsi = modem.getIMSI();
    String targetApn = config.apn;
    bool autoApn = false;
    if (targetApn.isEmpty() && imsi.length() >= 5) {
        targetApn = autoResolveApnFromImsi(imsi);
        autoApn = !targetApn.isEmpty();
    }
    _resolvedApn = targetApn.c_str();
    setCellularProvisioningSnapshot(imsi.length() >= 5, autoApn, false, _resolvedApn.c_str());

    // Backoff reconnect: up to 5 retries with exponential delay (fallback to IMSI APN on retry)
    int  retry   = 0;
    long delayMs = 2000;
    while (!modem.gprsConnect(targetApn.c_str(),
                              config.apn_user.c_str(),
                              config.apn_pass.c_str())) {
        retry++;
        if (retry == 2 && imsi.length() >= 5) {
            String autoApn = autoResolveApnFromImsi(imsi);
            if (!autoApn.isEmpty() && autoApn != targetApn) {
                Serial.printf("[SIM] Auto-resolving carrier APN from MCC/MNC (%s): %s\n",
                              imsi.substring(0, 5).c_str(), autoApn.c_str());
                targetApn = autoApn;
                _resolvedApn = targetApn.c_str();
            }
        }
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
    if (_rssi > 0 && _rssi != 99) _lastValidRssi = _rssi;
    _jammingDetected = false;
    Serial.printf("[SIM] Signal quality: %d (APN: %s)\n", _rssi, _resolvedApn.c_str());
    return true;
}

bool isSimConnected() {
    return modem.isGprsConnected();
}

int getSignalRSSI() {
    int csq = modem.getSignalQuality();
    bool reg = modem.isNetworkConnected();
    // Cellular RF Jamming Detection (GTJDC): abrupt signal collapse + registration loss
    if (_lastValidRssi >= 12 && (csq == 0 || csq == 99) && !reg) {
        _jammingDetected = true;
    } else if (reg && csq >= 5 && csq != 99) {
        _jammingDetected = false;
    }
    if (csq > 0 && csq != 99) _lastValidRssi = csq;
    _rssi = csq;
    return _rssi;
}

bool isSimJammingDetected() {
    return _jammingDetected;
}

std::string getSimResolvedApn() {
    return _resolvedApn;
}

std::string getSimLocalIp() {
    if (!modem.isGprsConnected()) return "0.0.0.0";
    return std::string(modem.localIP().toString().c_str());
}
