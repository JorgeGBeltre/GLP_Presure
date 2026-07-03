#pragma once

#include <string>

namespace glp::domain {

/** Logging. En el firmware envuelve Serial; en tests puede capturar o descartar. */
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(const std::string& msg) = 0;
    virtual void warn(const std::string& msg) = 0;
    virtual void error(const std::string& msg) = 0;
};

} // namespace glp::domain
