add_subdirectory(uno)

add_executable(uno-test uno-test.cc $<TARGET_OBJECTS:uno-dependencies>)
target_link_libraries(uno-test gtest::gtest Boost::log)

