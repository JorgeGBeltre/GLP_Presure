#pragma once
#include <Arduino.h>
#include "Domain/Interfaces/IButtonHandler.h"
#include "Domain/Interfaces/IIndicator.h"

void initWiFiAPProvisioning();
bool runWiFiAPProvisioning(unsigned long timeoutMs = 300000,
                           glp::domain::IIndicator* indicator = nullptr,
                           glp::domain::IButtonHandler* button = nullptr);
