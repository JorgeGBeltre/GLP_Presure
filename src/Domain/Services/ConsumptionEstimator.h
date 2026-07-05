#pragma once

namespace glp::domain {

/**
 * Estima el consumo de GLP (L/día) a partir de la caída de volumen en el tiempo,
 * y la autonomía restante. PURO, testeable en host. Estado mínimo: último volumen
 * y timestamp. Las recargas (volumen que sube) no cuentan como consumo.
 */
class ConsumptionEstimator {
public:
    /** Nueva medición de volumen (L) con el tiempo actual (ms, de IClock). */
    void update(float volumeLiters, unsigned long nowMs) {
        if (!seeded_) { seeded_ = true; lastV_ = volumeLiters; lastMs_ = nowMs; return; }
        const unsigned long dtMs = nowMs - lastMs_;
        if (dtMs == 0) return;

        const float dtDays = static_cast<float>(dtMs) / 86400000.0f; // ms → días
        const float rate   = (lastV_ - volumeLiters) / dtDays;       // L/día (positivo = consumo)
        if (rate > 0.0f) {
            ratePerDay_ = uninit_ ? rate : (kAlpha * rate + (1.0f - kAlpha) * ratePerDay_);
            uninit_ = false;
        }
        lastV_  = volumeLiters;
        lastMs_ = nowMs;
    }

    float litersPerDay() const { return uninit_ ? 0.0f : ratePerDay_; }

    /** Días restantes al consumo promedio (0 si aún no hay tasa). */
    float remainingDays(float currentVolumeLiters) const {
        const float q = litersPerDay();
        return (q > 0.0f) ? currentVolumeLiters / q : 0.0f;
    }

    void reset() { seeded_ = false; uninit_ = true; ratePerDay_ = 0.0f; lastV_ = 0.0f; lastMs_ = 0; }

private:
    static constexpr float kAlpha = 0.2f;   // suavizado EMA
    bool          seeded_     = false;
    bool          uninit_     = true;
    float         lastV_      = 0.0f;
    float         ratePerDay_ = 0.0f;
    unsigned long lastMs_     = 0;
};

} // namespace glp::domain
