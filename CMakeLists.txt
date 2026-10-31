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

# Disable fmt consteval and OS utilities to avoid clang 20/21 consteval issues
set(GEODE_DISABLE_FMT_CONSTEVAL ON CACHE BOOL "" FORCE)
set(FMT_OS OFF CACHE BOOL "Disable fmt OS support" FORCE)
set(FMT_DOCS OFF CACHE BOOL "Disable fmt docs" FORCE)
set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
set(FMT_MODULE OFF CACHE BOOL "Disable fmt modules" FORCE)
add_compile_definitions(FMT_USE_CONSTEVAL=0 FMT_CONSTEVAL=)

# Add Geode SDK
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode_sdk)

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
