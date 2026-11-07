cmake_minimum_required(VERSION 3.17)
project(ExampleAddon LANGUAGES CXX)

include(cmake/LegacyAddon.cmake)
legacy_addon(ExampleAddon src/Main.cpp)
