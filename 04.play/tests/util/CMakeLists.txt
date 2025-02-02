simple_gtest(log-test.cc util::log)

add_executable(util-str_split-test str_split-test.cc)
target_link_libraries(util-str_split-test gtest::gtest Boost::headers)
gtest_discover_tests(util-str_split-test)
