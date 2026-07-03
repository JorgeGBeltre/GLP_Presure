#pragma once

#include <string>

namespace glp::domain {

/** Sincronización horaria y timestamp ISO-8601 (implementado por NtpTimeProvider). */
class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;
    virtual bool sync() = 0;                 // arranca/valida NTP
    virtual std::string isoTimestamp() = 0;  // "YYYY-MM-DDTHH:MM:SS.mmmZ"
};

} // namespace glp::domain
