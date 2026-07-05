#pragma once

namespace glp::domain {

/**
 * Filtro de estado del tanque de GLP (opción B del diseño). PURO, testeable en host.
 *
 * Estado x = [nivel h (mm), temperatura T (°C)]. Proceso: random-walk (F = I).
 * Mediciones z = [h_us, T_pt1000, T_desdeP], donde T_desdeP = Antoine⁻¹(P_medida)
 * la calcula el llamador. Las tres medidas son lineales en el estado (H filas
 * [1,0], [0,1], [0,1]), así que el "EKF" se reduce a un Kalman lineal 2×3: la
 * presión aporta una 2ª observación de T y así mejora T (y densidad/masa).
 *
 * Densidad, volumen y masa son SALIDAS de (ĥ, T̂), nunca estados (evita la
 * covarianza singular). Actualización por medición escalar (R diagonal) con
 * forma de Joseph para mantener P positiva-definida en float.
 */
class StateEkf {
public:
    StateEkf(float qH, float qT, float rH, float rT, float rTfromP)
        : qH_(qH), qT_(qT), rH_(rH), rT_(rT), rTfromP_(rTfromP) {}

    void reset(float h0 = 0.0f, float t0 = 15.0f, float p0 = 1.0e4f) {
        xH_ = h0; xT_ = t0;
        p00_ = p0; p11_ = p0; p01_ = 0.0f;
    }

    /** Un ciclo: predicción (random-walk) + corrección con las medidas válidas. */
    void update(float hMeas, bool hValid,
                float tMeas, bool tValid,
                float tFromP, bool tFromPValid) {
        // Predicción: P += Q (F = I)
        p00_ += qH_;
        p11_ += qT_;
        // Correcciones escalares secuenciales (R diagonal ⇒ sin invertir matrices)
        if (hValid)      scalarUpdate(1.0f, 0.0f, hMeas,  rH_);
        if (tValid)      scalarUpdate(0.0f, 1.0f, tMeas,  rT_);
        if (tFromPValid) scalarUpdate(0.0f, 1.0f, tFromP, rTfromP_);
    }

    float level()    const { return xH_; }
    float temp()     const { return xT_; }
    float levelVar() const { return p00_; }
    float tempVar()  const { return p11_; }

private:
    /** Corrección escalar con H = [h0, h1], medición z, ruido r. Forma de Joseph. */
    void scalarUpdate(float h0, float h1, float z, float r) {
        const float Hx = h0 * xH_ + h1 * xT_;
        const float y  = z - Hx;                       // innovación
        const float PHt0 = p00_ * h0 + p01_ * h1;      // P Hᵀ
        const float PHt1 = p01_ * h0 + p11_ * h1;      // (P simétrica: p10 = p01)
        const float S  = h0 * PHt0 + h1 * PHt1 + r;    // escalar
        const float k0 = PHt0 / S;                     // ganancia
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

    float qH_, qT_, rH_, rT_, rTfromP_;
    float xH_  = 0.0f,   xT_  = 15.0f;
    float p00_ = 1.0e4f, p01_ = 0.0f, p11_ = 1.0e4f;
};

} // namespace glp::domain
