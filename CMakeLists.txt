cmake_minimum_required(VERSION 3.15)
project(ExampleAddon VERSION 1.0.0 LANGUAGES CXX)

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
        -Wall
        -Wextra
        -fPIC
        -fvisibility=hidden
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
