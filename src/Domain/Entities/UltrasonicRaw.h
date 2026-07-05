#pragma once

namespace glp::domain {

/**
 * Datos CRUDOS del ultrasónico: tiempos de vuelo + métricas de calidad. Ninguna
 * conversión física (velocidad del sonido / distancia) vive en Infraestructura;
 * todo se hace en el dominio (AcousticModel). Estructura pensada para el futuro:
 * soporta un reflector de referencia y calidad de eco aunque el hardware actual
 * no los provea todavía (refValid=false).
 */
struct UltrasonicRaw {
    float tFlightUs    = 0.0f;   // ida y vuelta al eco del líquido (µs)
    bool  echoValid    = false;  // hubo eco del líquido
    float tRefUs       = 0.0f;   // ida y vuelta al reflector de referencia (µs)
    bool  refValid     = false;  // hubo eco de referencia
    float echoStrength = 0.0f;   // calidad/amplitud del eco 0..1 (opcional)
};

} // namespace glp::domain
