#pragma once
#include <Arduino.h>

void initWiFiAPProvisioning();
bool runWiFiAPProvisioning(unsigned long timeoutMs = 300000);
