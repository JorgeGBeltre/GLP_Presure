#pragma once

#include <Arduino.h>

#include "Domain/Interfaces/ILogger.h"

namespace glp::infra {

/** ILogger sobre Serial. */
class SerialLogger : public domain::ILogger {
public:
    void info(const std::string& m) override  { Serial.printf("[INFO] %s\n", m.c_str()); }
    void warn(const std::string& m) override  { Serial.printf("[WARN] %s\n", m.c_str()); }
    void error(const std::string& m) override { Serial.printf("[ERR ] %s\n", m.c_str()); }
};

} // namespace glp::infra
