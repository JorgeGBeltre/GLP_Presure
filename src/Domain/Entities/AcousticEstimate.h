#pragma once

namespace glp::domain {

/** Resultado del modelo acústico: velocidad del sonido estimada + distancia + salud. */
struct AcousticEstimate {
    float cMs        = 343.0f;  // velocidad del sonido estimada (m/s)
    float sigmaMs    = 0.0f;    // incertidumbre asociada (±, m/s)
    bool  reflector  = false;   // true si la fuente primaria fue el reflector de referencia
    bool  healthy    = true;    // reflector y modelo de gas coinciden (dentro de tolerancia)
    float distanceMm = 0.0f;    // distancia al líquido (mm)
    bool  valid      = false;   // hubo eco de líquido válido
};

} // namespace glp::domain
