#pragma once

namespace glp::domain {

/** Métricas de sistema incluidas en la telemetría. */
struct SystemMetrics {
    int           rssi     = 0;
    unsigned long freeHeap = 0;
    unsigned long uptime   = 0;
};

} // namespace glp::domain
