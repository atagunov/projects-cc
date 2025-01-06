add_subdirectory(uno)

simple_module_test(uno-test.cc gtest::gtest)
gtest_discover_tests(uno-test)
