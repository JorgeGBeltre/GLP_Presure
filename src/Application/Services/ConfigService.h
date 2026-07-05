#pragma once

#include "Application/Dto/RemoteConfigUpdate.h"
#include "Domain/Entities/DeviceConfig.h"
#include "Domain/Entities/TankDimensions.h"
#include "Domain/Interfaces/IConfigRepository.h"

namespace glp::app {

/**
 * Dueña de la configuración viva en RAM. Carga desde el repositorio, aplica
 * actualizaciones remotas y persiste. El resto de servicios leen la config
 * a través de una referencia const provista por este servicio.
 */
class ConfigService {
public:
    explicit ConfigService(domain::IConfigRepository& repo) : repo_(repo) {}

    /** Carga la config persistida a RAM. Devuelve false si no había nada. */
    bool load();

    bool isProvisioned() { return repo_.isProvisioned(); }

    const domain::DeviceConfig& config() const { return config_; }
    domain::DeviceConfig&       mutableConfig() { return config_; }

    domain::TankDimensions tankDimensions() const {
        return domain::TankDimensions::fromCm(config_.tankRadiusCm,
                                              config_.tankLengthCm);
    }

    const domain::SensorCalibration& sensorCalibration() const { return config_.calibration; }
    float propaneFraction() const { return config_.propaneFraction; }
    float sensorAxialCm()   const { return config_.sensorAxialCm; }

    /**
     * Aplica los campos presentes en `update`, persiste y devuelve true si algo
     * cambió y se guardó correctamente.
     */
    bool applyRemoteUpdate(const RemoteConfigUpdate& update);

    /** Persiste la config viva tal cual. */
    bool save() { return repo_.save(config_); }

private:
    domain::IConfigRepository& repo_;
    domain::DeviceConfig       config_{};
};

} // namespace glp::app
