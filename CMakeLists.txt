cmake_minimum_required(VERSION 3.14)
project(glp_sensor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────────────────────────────────────────────────────────
# Opciones
# ─────────────────────────────────────────────────────────────────────────────
# HOST_BUILD=ON  -> compila SÓLO Domain + Application (C++17 puro) y los tests.
#                   Es lo que corre en tu PC / CI, sin ESP32.
# HOST_BUILD=OFF -> además intenta compilar Infrastructure + Presentation. Esto
#                   requiere un toolchain con las cabeceras de Arduino/ESP32; en
#                   la práctica el firmware real se compila con PlatformIO
#                   (platformio.ini), no con este CMake. Se deja el target por
#                   completitud / cross-builds.
option(HOST_BUILD       "Build pure Domain+Application for host (no Arduino)" ON)
option(GLP_BUILD_TESTS  "Build host unit tests (requires HOST_BUILD)"          ON)

# ─────────────────────────────────────────────────────────────────────────────
# Fuentes por capa
# ─────────────────────────────────────────────────────────────────────────────
# Domain es header-only (entidades + interfaces + algoritmos puros): no aporta
# .cpp. Application sí tiene unidades de traducción.
set(GLP_APPLICATION_SOURCES
    src/Application/Services/ConfigService.cpp
    src/Application/Services/ConnectivityService.cpp
    src/Application/Services/TelemetryService.cpp
    src/Application/Controllers/FirmwareController.cpp
)

# Infrastructure: implementaciones concretas que dependen de Arduino/ESP32 y de
# librerías externas (ArduinoJson, ESP32_MQTTv5, TinyGSM, ...). Sólo se añaden en
# build de target.
set(GLP_INFRASTRUCTURE_SOURCES
    src/Infrastructure/Messaging/Mqttv5Transport.cpp
    src/Infrastructure/Net/NetworkClient.cpp
    src/Infrastructure/Serialization/ArduinoJsonPayloadSerializer.cpp
    src/Infrastructure/System/EspDeviceRepository.cpp
    src/Infrastructure/Time/NtpTimeProvider.cpp
)

# ─────────────────────────────────────────────────────────────────────────────
# Biblioteca núcleo: Domain + Application (+ Infrastructure en target)
# ─────────────────────────────────────────────────────────────────────────────
add_library(glp_core STATIC ${GLP_APPLICATION_SOURCES})
target_include_directories(glp_core PUBLIC src)

if(NOT HOST_BUILD)
    target_sources(glp_core PRIVATE ${GLP_INFRASTRUCTURE_SOURCES})

    # Dependencias externas SÓLO del build de target. En host, el núcleo no tiene
    # ninguna (la serialización vive tras IPayloadSerializer). Ejemplo de cómo se
    # enlazarían si se hiciera un cross-build fuera de PlatformIO:
    #
    #   find_package(ArduinoJson CONFIG REQUIRED)
    #   target_link_libraries(glp_core PUBLIC ArduinoJson)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Ejecutable de firmware (Presentation). En hardware real usar PlatformIO.
# ─────────────────────────────────────────────────────────────────────────────
if(NOT HOST_BUILD)
    add_executable(glp_firmware src/Presentation/main.cpp)
    target_link_libraries(glp_firmware PRIVATE glp_core)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Tests de host
# ─────────────────────────────────────────────────────────────────────────────
if(GLP_BUILD_TESTS AND HOST_BUILD)
    enable_testing()
    add_subdirectory(tests)
endif()
