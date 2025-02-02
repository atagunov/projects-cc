cmake_minimum_required(VERSION 3.31)

project(04.play VERSION 0.1.0 LANGUAGES CXX)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wctad-maybe-unsupported")
endif()

include(cmake/SimpleModules.cmake)
