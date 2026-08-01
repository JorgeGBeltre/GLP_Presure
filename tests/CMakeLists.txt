# ─────────────────────────────────────────────────────────────────────────────
# Tests de host (GoogleTest). Sólo dependen de glp_core (Domain+Application) y de
# glp_mocks (dobles de las interfaces de Domain). Cero hardware.
# ─────────────────────────────────────────────────────────────────────────────

# --- GoogleTest: usar el instalado o descargarlo con FetchContent ------------
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# --- Biblioteca de mocks: los dobles que implementan las interfaces de Domain -
# Header-only, así que es una INTERFACE library que expone el include dir y se
# enlaza contra el núcleo.
add_library(glp_mocks INTERFACE)
target_include_directories(glp_mocks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(glp_mocks INTERFACE glp_core)

# --- Helper: registra un test sin repetir boilerplate ------------------------
# glp_add_test(<nombre> <fuente> [<fuente> ...])
function(glp_add_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE glp_core glp_mocks GTest::gtest_main)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# --- Un ejecutable de test por servicio de Application (+ el algoritmo puro) --
glp_add_test(test_sensor_fusion       test_sensor_fusion.cpp)
glp_add_test(test_sensor_calibrator   test_sensor_calibrator.cpp)
glp_add_test(test_tank_geometry       test_tank_geometry.cpp)
glp_add_test(test_calibrating_sensor_reader test_calibrating_sensor_reader.cpp)
glp_add_test(test_lpg_thermo          test_lpg_thermo.cpp)
glp_add_test(test_consumption_estimator test_consumption_estimator.cpp)
glp_add_test(test_sensor_health       test_sensor_health.cpp)
glp_add_test(test_tilt_correction     test_tilt_correction.cpp)
glp_add_test(test_state_ekf           test_state_ekf.cpp)
glp_add_test(test_acoustic_model      test_acoustic_model.cpp)
glp_add_test(test_config_service      test_config_service.cpp)
glp_add_test(test_connectivity_service test_connectivity_service.cpp)
glp_add_test(test_telemetry_service   test_telemetry_service.cpp)
glp_add_test(test_firmware_controller test_firmware_controller.cpp)
