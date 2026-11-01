cmake_minimum_required(VERSION 3.14)
project(glp_sensor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HOST_BUILD       "Build pure Domain+Application for host (no Arduino)" ON)
option(GLP_BUILD_TESTS  "Build host unit tests (requires HOST_BUILD)"          ON)

set(GLP_APPLICATION_SOURCES
    src/Application/Services/ConfigService.cpp
    src/Application/Services/ConnectivityService.cpp
    src/Application/Services/TelemetryService.cpp
    src/Application/Services/CryptoUtils.cpp
    src/Application/Controllers/FirmwareController.cpp
)

set(GLP_INFRASTRUCTURE_SOURCES
    src/Infrastructure/Button/GpioButton.cpp
    src/Infrastructure/Messaging/Mqttv5Transport.cpp
    src/Infrastructure/Net/NetworkClient.cpp
    src/Infrastructure/Net/LanZeroconfServer.cpp
    src/Infrastructure/Serialization/ArduinoJsonPayloadSerializer.cpp
    src/Infrastructure/System/EspDeviceRepository.cpp
    src/Infrastructure/Time/NtpTimeProvider.cpp
)

add_library(glp_core STATIC ${GLP_APPLICATION_SOURCES})
target_include_directories(glp_core PUBLIC src)

if(NOT HOST_BUILD)
    target_sources(glp_core PRIVATE ${GLP_INFRASTRUCTURE_SOURCES})
endif()

if(NOT HOST_BUILD)
    add_executable(glp_firmware src/Presentation/main.cpp)
    target_link_libraries(glp_firmware PRIVATE glp_core)
endif()

if(GLP_BUILD_TESTS AND HOST_BUILD)
    enable_testing()
    add_subdirectory(tests)
endif()
