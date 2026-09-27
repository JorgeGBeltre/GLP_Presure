#pragma once

#include <algorithm>
#include <cmath>

namespace glp::domain {

/**
 * Filtro de Kalman Extendido (EKF) de estado conjunto para tanque de GLP.
 *
 * Vector de estado:
 *   x = [h (mm), T (°C)]ᵀ   (nivel del líquido h, temperatura T).
 * Modelo de proceso:
 *   Random-walk: x_k = x_{k-1} + w_k,  w_k ~ N(0, Q),  F = I₂.
 *   Predicción de covarianza: P ← P + Q.
 *
 * Modelo de mediciones (desacopladas/ruido R diagonal ⇒ actualizaciones escalares secuenciales):
 *   1. Nivel ultrasónico: z_h = h + v_h,  H_h = [1, 0]  (lineal).
 *   2. Temperatura Pt1000: z_T = T + v_T,  H_T = [0, 1]  (lineal).
 *   3. Presión de vapor saturado: z_P = P_sat(T) + v_P    (NO-LINEAL).
 *      Jacobiano: H_P = [0, dP_sat/dT(T̂)].
 *      Innovación: y_P = z_P - P_sat(T̂).
 *      Compuerta de Mahalanobis (3-sigma): y_P² / S_P ≤ 9.0 para rechazar anomalías/fugas.
 *
 * Todas las correcciones usan la forma simétrica de Joseph:
 *   P = (I - K H) P (I - K H)ᵀ + K R Kᵀ
 * garantizando que la matriz de covarianza P permanezca definida-positiva y numéricamente
 * estable incluso con aritmética float de 32 bits.
 */
class StateEkf {
public:
    /**
     * @param qH       Varianza del ruido de proceso del nivel (mm²/ciclo)
     * @param qT       Varianza del ruido de proceso de la temperatura (°C²/ciclo)
     * @param rH       Varianza de medición del sensor ultrasónico (mm²)
     * @param rT       Varianza de medición del sensor Pt1000 (°C²)
     * @param rP       Varianza de medición del sensor de presión (bar²)
     */
    StateEkf(float qH, float qT, float rH, float rT, float rP)
        : qH_(qH), qT_(qT), rH_(rH), rT_(rT), rP_(rP) {}

    void reset(float h0 = 0.0f, float t0 = 15.0f, float p0 = 1.0e4f) {
        xH_ = h0;
        xT_ = t0;
        p00_ = p0;
        p11_ = p0;
        p01_ = 0.0f;
    }

    /**
     * Ciclo EKF completo NO-LINEAL:
     * Predicción (F=I, P+=Q) + Corrección con nivel, temperatura y presión física (bar)
     * mediante la función no-lineal P_sat(T) y su Jacobiano dP/dT(T̂).
     */
    void updateNonlinear(float hMeas, bool hValid,
                         float tMeas, bool tValid,
                         float pMeasBar, bool pValid,
                         float pSatBar, float dP_dT,
                         bool gateOutliers = true) {
        // 1. Predicción: P += Q (F = I)
        p00_ += qH_;
        p11_ += qT_;

        // 2. Medición lineal de nivel: H = [1, 0], hx = xH_
        if (hValid) {
            scalarUpdate(1.0f, 0.0f, xH_, hMeas, rH_);
        }

        // 3. Medición lineal de temperatura Pt1000: H = [0, 1], hx = xT_
        if (tValid) {
            scalarUpdate(0.0f, 1.0f, xT_, tMeas, rT_);
        }

        // 4. Medición NO-LINEAL de presión de vapor: H = [0, dP/dT], hx = pSatBar
        if (pValid && dP_dT > 1e-4f) {
            const float y = pMeasBar - pSatBar;
            const float S = dP_dT * (p01_ * 0.0f + p11_ * dP_dT) + rP_;
            // Compuerta de innovación (Mahalanobis / gating 3-sigma: chi2 1-GL 99.7%)
            const bool passGate = !gateOutliers || (S > 0.0f && (y * y / S) <= 9.0f);
            if (passGate) {
                scalarUpdate(0.0f, dP_dT, pSatBar, pMeasBar, rP_);
            }
        }

        // Suelo de varianza numérica para estabilidad float
        p00_ = std::max(p00_, 1e-6f);
        p11_ = std::max(p11_, 1e-6f);
    }

    /**
     * Sobrecarga legacy: corrección escalar lineal con T_desdeP calculada previamente.
     * Mantiene 100% de compatibilidad hacia atrás con tests existentes.
     */
    void update(float hMeas, bool hValid,
                float tMeas, bool tValid,
                float tFromP, bool tFromPValid) {
        p00_ += qH_;
        p11_ += qT_;
        if (hValid)      scalarUpdate(1.0f, 0.0f, xH_, hMeas,  rH_);
        if (tValid)      scalarUpdate(0.0f, 1.0f, xT_, tMeas,  rT_);
        if (tFromPValid) scalarUpdate(0.0f, 1.0f, xT_, tFromP, rP_);

        p00_ = std::max(p00_, 1e-6f);
        p11_ = std::max(p11_, 1e-6f);
    }

    float level()    const { return xH_; }
    float temp()     const { return xT_; }
    float levelVar() const { return p00_; }
    float tempVar()  const { return p11_; }
    float covarHT()  const { return p01_; }

    float processNoiseH() const { return qH_; }
    float processNoiseT() const { return qT_; }
    float measNoiseH()    const { return rH_; }
    float measNoiseT()    const { return rT_; }
    float measNoiseP()    const { return rP_; }

private:
    /**
     * Corrección escalar EKF general:
     * H = [h0, h1], medición esperada hx, medición observada z, ruido r.
     * Forma de Joseph: P = (I - K H) P (I - K H)ᵀ + K r Kᵀ.
     */
    void scalarUpdate(float h0, float h1, float hx, float z, float r) {
        const float y    = z - hx;                     // innovación
        const float PHt0 = p00_ * h0 + p01_ * h1;      // P Hᵀ
        const float PHt1 = p01_ * h0 + p11_ * h1;      // (P simétrica: p10 = p01)
        const float S    = h0 * PHt0 + h1 * PHt1 + r;  // escalar de innovación
        if (S <= 1e-9f) return;                        // guarda contra división por cero

        const float k0 = PHt0 / S;                     // ganancia de Kalman
        const float k1 = PHt1 / S;

        xH_ += k0 * y;
        xT_ += k1 * y;

        // Joseph: P = A P Aᵀ + K r Kᵀ,  A = I − K H
        const float a00 = 1.0f - k0 * h0, a01 = -k0 * h1;
        const float a10 = -k1 * h0,       a11 = 1.0f - k1 * h1;
        const float ap00 = a00 * p00_ + a01 * p01_;
        const float ap01 = a00 * p01_ + a01 * p11_;
        const float ap10 = a10 * p00_ + a11 * p01_;
        const float ap11 = a10 * p01_ + a11 * p11_;

        p00_ = ap00 * a00 + ap01 * a01 + k0 * r * k0;
        p01_ = ap00 * a10 + ap01 * a11 + k0 * r * k1;
        p11_ = ap10 * a10 + ap11 * a11 + k1 * r * k1;
    }

    float qH_, qT_, rH_, rT_, rP_;
    float xH_  = 0.0f,   xT_  = 15.0f;
    float p00_ = 1.0e4f, p01_ = 0.0f, p11_ = 1.0e4f;
};

} // namespace glp::domain

