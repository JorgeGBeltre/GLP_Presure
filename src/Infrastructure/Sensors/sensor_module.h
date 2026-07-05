#pragma once
#include <Arduino.h>

void initSensors();
bool readUltrasonicRaw(float& outMm);
float readPressureRawBar();
float readTemperatureRawC();
