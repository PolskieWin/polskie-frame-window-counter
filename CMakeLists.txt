cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PolskieFrameWindowCounter VERSION 3.0.0)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "Unable to find Geode SDK! Please define the GEODE_SDK environment variable "
        "to point to the Geode SDK installation directory."
    )
endif()

# Add Geode SDK
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode_sdk)

CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.11.3
)

# Define the mod library target
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/FrameWindowAnalyzer.cpp
    src/FrameWindowOverlay.cpp
    src/MacroParser.cpp
    src/FWCSettingsPopup.cpp
    src/FPSoundManager.cpp
)

# Setup Geode mod
setup_geode_mod(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
