cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PolskieFrameWindowCounter VERSION 3.0.0)

# Find Geode SDK
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "Unable to find Geode SDK! Please define the GEODE_SDK environment variable "
        "to point to the Geode SDK installation directory."
    )
endif()

# Add Geode cmake utilities
include($ENV{GEODE_SDK}/cmake/Geode.cmake)

# Setup the Geode mod
setup_geode_mod(${PROJECT_NAME})

# Add all source files
target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/FrameWindowAnalyzer.cpp
    src/FrameWindowOverlay.cpp
    src/MacroParser.cpp
    src/FWCSettingsPopup.cpp
    src/FPSoundManager.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)
