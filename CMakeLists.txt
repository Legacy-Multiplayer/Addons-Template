cmake_minimum_required(VERSION 3.20)
project(ExampleAddon LANGUAGES CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDK/CMakeLists.txt")
    message(FATAL_ERROR "Initialize Addon-SDK: git submodule update --init --recursive")
endif()
add_subdirectory(SDK)
include(SDK/cmake/Plugin.cmake)
legacy_plugin(ExampleAddon src/Main.cpp)
