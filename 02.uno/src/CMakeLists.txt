include_directories(".")

add_subdirectory(uno)

simple_module(uno.cc Boost::log)

add_executable(main main.cc)
target_link_libraries(main uno)