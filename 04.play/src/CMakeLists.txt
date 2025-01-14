add_subdirectory(util)

add_executable(spans spans.cc)
add_executable(experimental-features experimental-features.cc)

add_executable(exceptions exceptions.cc)
target_link_libraries(exceptions util::log)
