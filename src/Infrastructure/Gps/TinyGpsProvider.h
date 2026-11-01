#pragma once

#include <cmath>
#include "Domain/Interfaces/IGpsProvider.h"
#include "Infrastructure/Gps/gps_module.h"   // driver existente (initGPS, readGPS, getLatitude, ...)

namespace glp::infra {

/**
 * IGpsProvider sobre el driver procedural gps/gps_module (TinyGPS++) enriquecido
 * con filtro de deriva estática (GTRDF, radio 30 m) y detección de movimiento/robo
 * del tanque estacionario (GTTOW / GTGEO, umbral 50 m).
 */
class TinyGpsProvider : public domain::IGpsProvider {
public:
    void begin() override { initGPS(); }
    void feed() override { readGPS(); }

    domain::GpsFix location() override {
        domain::GpsFix f;
        f.valid     = isGPSValid();
        f.timestamp = getGPSTimestamp().c_str();
        if (!f.valid) {
            return f;
        }

        double rawLat = getLatitude();
        double rawLon = getLongitude();

        if (!hasAnchor_) {
            hasAnchor_ = true;
            anchorLat_ = rawLat;
            anchorLon_ = rawLon;
        }

        const float distM = haversineMeters(anchorLat_, anchorLon_, rawLat, rawLon);
        f.displacementM = distM;

        if (distM <= 30.0f) {
            // Filtro de deriva estática (GTRDF): fijar coordenada del tanque estacionario
            f.latitude      = anchorLat_;
            f.longitude     = anchorLon_;
            f.driftFiltered = true;
            f.tamperMoved   = false;
        } else {
            f.latitude      = rawLat;
            f.longitude     = rawLon;
            f.driftFiltered = false;
            f.tamperMoved   = (distM >= 50.0f);
        }
        return f;
    }

private:
    static float haversineMeters(double lat1, double lon1, double lat2, double lon2) {
        constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
        constexpr double kEarthRadiusM = 6371000.0;
        double dLat = (lat2 - lat1) * kDegToRad;
        double dLon = (lon2 - lon1) * kDegToRad;
        double a = std::sin(dLat * 0.5) * std::sin(dLat * 0.5) +
                   std::cos(lat1 * kDegToRad) * std::cos(lat2 * kDegToRad) *
                   std::sin(dLon * 0.5) * std::sin(dLon * 0.5);
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        return static_cast<float>(kEarthRadiusM * c);
    }

    bool   hasAnchor_ = false;
    double anchorLat_ = 0.0;
    double anchorLon_ = 0.0;
};

} // namespace glp::infra
