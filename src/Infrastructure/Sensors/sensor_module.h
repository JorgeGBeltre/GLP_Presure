#pragma once
#include <Arduino.h>

void initSensors();
bool readUltrasonicTimeUs(float& outUs);
float readPressureRawBar();
float readTemperatureRawC();
