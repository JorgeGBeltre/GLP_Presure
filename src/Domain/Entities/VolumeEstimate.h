#pragma once

namespace glp::domain {

/**
 * Resultado del pipeline de fusión de sensores (antes `GlpSensorFusion::FusionResult`).
 * Contiene los valores filtrados + volumen calculado + diagnóstico.
 */
struct VolumeEstimate {
    float levelMm             = 0.0f; // altura filtrada del líquido (mm)
    float pressureBar         = 0.0f; // presión filtrada (bar)
    float tempCelsius         = 0.0f; // temperatura suavizada (°C)
    float volumeLiters        = 0.0f; // volumen (L)
    float gallons             = 0.0f; // galones de GLP
    float percentage          = 0.0f; // porcentaje de llenado
    float densityKgL          = 0.0f; // densidad del GLP a temperatura actual
    float massKg              = 0.0f; // masa de GLP = densidad × volumen
    float volume15Liters      = 0.0f; // volumen corregido a 15 °C (facturación / VCF)
    bool  anomaly             = false; // presión y nivel inconsistentes
    float kalmanUncertaintyMm = 0.0f; // incertidumbre actual del filtro de nivel
};

} // namespace glp::domain
