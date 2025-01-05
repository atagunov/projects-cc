add_subdirectory(uno)

#add_executable(uno-test uno-test.cc $<TARGET_OBJECTS:uno-dependencies>)

simple_module_test(uno-test.cc gtest::gtest)
gtest_discover_tests(uno-test)
