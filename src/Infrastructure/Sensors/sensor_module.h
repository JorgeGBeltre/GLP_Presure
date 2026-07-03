#pragma once
#include <Arduino.h>

void initSensors();
float readUltrasonicRawMM();
float readPressureRawBar();
float readTemperatureRawC();
