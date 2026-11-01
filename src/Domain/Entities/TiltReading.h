#pragma once

namespace glp::domain {

/** Lectura de inclinación e impacto sísmico/mecánico del tanque (de un IMU). */
struct TiltReading {
    float pitchRad = 0.0f;            // inclinación del EJE del tanque (la que sesga el nivel axial)
    float rollRad  = 0.0f;            // rotación de la SECCIÓN (volumen invariante)
    float accelMagnitudeG = 1.0f;     // magnitud del vector 3D de aceleración en unidades g (GTCRA)
    bool  seismicImpactDetected = false; // impacto mecánico o sismo detectado (|g - 1.0| >= 1.2g)
    bool  valid    = false;           // false ⇒ sin corrección (sin IMU o lectura no fiable)
};

} // namespace glp::domain
