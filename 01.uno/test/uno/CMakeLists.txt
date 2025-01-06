# uno/util-test.cc has an #include of uno/util.cc which is why we use
# $<TARGET_OBJECTS:uno-util-dependencies> not $<TARGET_OBJECTS:uno-util>

# add_executable(uno-util-test util-test.cc $<TARGET_OBJECTS:uno-util-dependencies>)
# target_link_libraries(uno-util-test gtest::gtest Boost::log global-includes)

simple_gtest(util-test.cc)
