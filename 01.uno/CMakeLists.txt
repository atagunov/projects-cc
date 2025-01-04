cmake_minimum_required(VERSION 3.31)

project(uno VERSION 0.1.0 LANGUAGES CXX)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

# this project builds an executable; .hh files are private to the project and are co-located with .cc under src/
add_library(global-includes INTERFACE)
target_include_directories(global-includes INTERFACE "${PROJECT_SOURCE_DIR}/src")

include("SimpleModule.cmake")

add_subdirectory(src)
add_subdirectory(test)

install(TARGETS uno-main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)


