cmake_minimum_required(VERSION 3.15)

# Default to 32-bit (Win32) generator platform on Windows if not specified
if(WIN32 AND NOT DEFINED CMAKE_GENERATOR_PLATFORM)
    set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "Target architecture" FORCE)
endif()

project(ExampleAddon VERSION 1.0.0 LANGUAGES CXX)

# Enforce 32-bit (x86) architecture to match Legacy Server
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Legacy Addon must be compiled for 32-bit (x86) to match the server! Please configure with: cmake -B build -A Win32 (or delete the existing build directory and reconfigure)")
endif()

# =====================================================================
# C++ Standard Configuration
# =====================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =====================================================================
# Addon Shared Library Target
# =====================================================================

add_library(${PROJECT_NAME} SHARED
    src/Main.cpp
)

# =====================================================================
# Include Directories
# =====================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# =====================================================================
# Compiler Options & Definitions
# =====================================================================

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /utf-8
        /permissive-
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -m32
        -Wall
        -Wextra
        -fPIC
        -fvisibility=hidden
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        -m32
    )
endif()

# =====================================================================
# Output Properties
# =====================================================================

# Ensures clean output filenames without default "lib" prefix on Unix
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ExampleAddon"
)
