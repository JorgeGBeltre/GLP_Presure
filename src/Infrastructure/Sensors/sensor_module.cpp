#include "sensor_module.h"
#include "Infrastructure/Config/BoardConfig.h"

// Pines centralizados en BoardConfig.h (válidos para el S3).
static constexpr uint8_t TRIG_PIN     = glp::board::TRIG_PIN;
static constexpr uint8_t ECHO_PIN     = glp::board::ECHO_PIN;
static constexpr uint8_t PRESSURE_PIN = glp::board::PRESSURE_PIN;
static constexpr uint8_t TEMP_PIN     = glp::board::TEMP_PIN;

void initSensors() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    // ADC configuration for pressure and temp if needed
    analogReadResolution(12);
}

// Devuelve false si el eco expira (pulseIn==0). La distancia usa la c del aire;
// la corrección por velocidad del sonido es Tier 3.
bool readUltrasonicRaw(float& outMm) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
    if (duration == 0) { outMm = 0.0f; return false; } // timeout -> inválido

    // speed of sound = 343 m/s = 0.343 mm/us
    outMm = (duration * 0.343f) / 2.0f;
    return true;
}

float readPressureRawBar() {
    int raw = analogRead(PRESSURE_PIN);
    // Dummy conversion logic. Requires calibration
    // Example: 0-3.3V corresponds to 0-4095 ADC and 0-10 bar
    float voltage = (raw / 4095.0f) * 3.3f;
    float pressure = voltage * (10.0f / 3.3f); 
    return pressure;
}

float readTemperatureRawC() {
    int raw = analogRead(TEMP_PIN);
    // Dummy thermistor logic.
    float tempC = 25.0f; // placeholder
    return tempC;
}
