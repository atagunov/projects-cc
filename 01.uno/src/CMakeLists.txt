add_subdirectory(uno)

# uno.cc represents the application as a whole ("uno" is the name of the project)
# it is separate from main.cc to facilitate unit testing

simple_module(uno.cc uno-util)

add_executable(uno-main main.cc $<TARGET_OBJECTS:uno>)
target_link_libraries(uno-main Boost::log)
