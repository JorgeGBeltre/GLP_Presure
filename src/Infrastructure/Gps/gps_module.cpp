#include "gps_module.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "Infrastructure/Config/BoardConfig.h"

// Pines centralizados en BoardConfig.h (válidos para el S3).
static constexpr uint8_t GPS_RX_PIN = glp::board::GPS_RX_PIN;
static constexpr uint8_t GPS_TX_PIN = glp::board::GPS_TX_PIN;
#define GPS_BAUD   9600

TinyGPSPlus   gps;
HardwareSerial gpsSerial(2);

static float   _latitude  = 0.0f;
static float   _longitude = 0.0f;
static bool    _valid     = false;
static String  _timestamp = "";

void initGPS() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[GPS] UART2 initialized @ " + String(GPS_BAUD) + " baud");
}

bool readGPS() {
    unsigned long start = millis();
    while (millis() - start < 200) {           // process 200ms of NMEA data
        while (gpsSerial.available()) {
            gps.encode(gpsSerial.read());
        }
    }

    if (gps.location.isUpdated() && gps.location.isValid()) {
        _latitude  = gps.location.lat();
        _longitude = gps.location.lng();
        _valid     = true;

        char buf[25];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
        _timestamp = String(buf);
    } else {
        _valid = false;
    }

    return _valid;
}

float getLatitude()       { return _latitude; }
float getLongitude()      { return _longitude; }
bool  isGPSValid()        { return _valid; }
String getGPSTimestamp()  { return _valid ? _timestamp : "1970-01-01T00:00:00Z"; }
