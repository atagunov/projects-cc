cmake_minimum_required(VERSION 3.31)

project(template VERSION 0.1.0 LANGUAGES CXX)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

include(cmake/SimpleModules.cmake)
