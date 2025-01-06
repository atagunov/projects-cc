cmake_minimum_required(VERSION 3.31)

project(uno VERSION 0.2.0 LANGUAGES CXX)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

include(cmake/SimpleModules.cmake)

add_subdirectory(src)
add_subdirectory(tests)
