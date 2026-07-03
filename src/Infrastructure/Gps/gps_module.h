#pragma once
#include <Arduino.h>

void initGPS();
bool readGPS();
float getLatitude();
float getLongitude();
bool isGPSValid();
String getGPSTimestamp();
