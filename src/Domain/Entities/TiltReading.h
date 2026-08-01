#pragma once

namespace glp::domain {

/** Lectura de inclinación del tanque (de un IMU). Ángulos en radianes. */
struct TiltReading {
    float pitchRad = 0.0f;   // inclinación del EJE del tanque (la que sesga el nivel axial)
    float rollRad  = 0.0f;   // rotación de la SECCIÓN (volumen invariante)
    bool  valid    = false;  // false ⇒ sin corrección (sin IMU o lectura no fiable)
};

} // namespace glp::domain
