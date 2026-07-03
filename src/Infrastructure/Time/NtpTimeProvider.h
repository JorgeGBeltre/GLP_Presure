#pragma once

#include "Domain/Interfaces/ITimeProvider.h"

namespace glp::infra {

/** ITimeProvider vía SNTP (configTime). Devuelve timestamp UTC ISO-8601. */
class NtpTimeProvider : public domain::ITimeProvider {
public:
    bool sync() override;               // registra los servidores SNTP (no bloqueante)
    std::string isoTimestamp() override;
};

} // namespace glp::infra
