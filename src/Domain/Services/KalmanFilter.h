#pragma once

namespace glp::domain {

/**
 * Filtro de Kalman escalar. Versión PURA (sin Arduino, sin heap).
 * Idéntico en matemática al antiguo `KalmanFilter` de src/kalman/kalman_filter.h,
 * pero sin ninguna dependencia de hardware, de modo que se testea en host.
 */
class KalmanFilter {
public:
    /**
     * @param q  Ruido del proceso (variabilidad física esperada entre muestras)
     * @param r  Ruido de medición (varianza del sensor)
     * @param x0 Valor inicial estimado
     * @param p0 Incertidumbre inicial
     */
    KalmanFilter(float q, float r, float x0 = 0.0f, float p0 = 1.0f)
        : q_(q), r_(r), x_(x0), p_(p0) {}

    /** Actualiza con una nueva medición `z` y devuelve la estimación filtrada. */
    float update(float z) {
        // Predicción
        p_ = p_ + q_;
        // Ganancia de Kalman
        const float k = p_ / (p_ + r_);
        // Corrección
        x_ = x_ + k * (z - x_);
        p_ = (1.0f - k) * p_;
        return x_;
    }

    float estimate()    const { return x_; }
    float uncertainty() const { return p_; }

    void reset(float x0, float p0 = 1.0f) { x_ = x0; p_ = p0; }
    void setProcessNoise(float q) { q_ = q; }

private:
    float q_;
    float r_;
    float x_;
    float p_;
};

} // namespace glp::domain
