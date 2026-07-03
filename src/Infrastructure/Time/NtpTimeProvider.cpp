#include "Infrastructure/Time/NtpTimeProvider.h"

#include <Arduino.h>
#include <time.h>

namespace glp::infra {

bool NtpTimeProvider::sync() {
    // UTC (offset 0): el timestamp se emite con sufijo 'Z'. SNTP resuelve async.
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    return true;
}

std::string NtpTimeProvider::isoTimestamp() {
    time_t now = time(nullptr);

    if (now < 1000000000) {   // aún sin hora real: fallback basado en uptime
        const unsigned long ms = millis();
        char fb[48];
        snprintf(fb, sizeof(fb), "1970-01-01T%02lu:%02lu:%02lu.%03luZ",
                 (ms / 3600000UL) % 24, (ms / 60000UL) % 60,
                 (ms / 1000UL) % 60, ms % 1000UL);
        return fb;
    }

    struct tm t;
    gmtime_r(&now, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &t);

    char full[48];
    snprintf(full, sizeof(full), "%s.%03luZ", buf, (unsigned long)(millis() % 1000));
    return full;
}

} // namespace glp::infra
