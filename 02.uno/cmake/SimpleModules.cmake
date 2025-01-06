# This project is using a "pseudo-module" hierarchy
# Actual C++20 modules are perceived as not being mature enough for prod use at the moment
#
# Files under "src" folder when including headers from their own project spell out full path relative to "src" folder
# For example in order to include "src/uno/util.hh" which contains "public interface" for "src/uno/util.cc"
# we write #include <uno/util.hh>
#
# Files under "tests" can do the same but additionally they can include headers sitting under "tests"
# So in order to include "tests/uno/test_utils/fs.hh"
# we write #include <uno/test_utils/fs.hh>
# which might contain public interface for "tests/uno/test_utils/fs.cc"
#
# By another convention "tests/uno/test_utils/fs.hh" will be placing all names into "uno::test_utils::fs" namespace
# This means that we cannot really use dashes in "test-utils" here
#
# We provide the following function for including .cc files into the project
# "simple_module" - for "src/uno/util.cc" (which becomes "uno-util" target, for convenience aliased by "uno::util")
# "simple_test_module" - for "tests/uno/test-util/fs.cc" (which becomes "uno-test_util-fs" target, aliased by "uno::test_util::fs")
# "simple_gtest" - for "tests/uno/util-test.cc" (which becomes "uno-util-test" target, note that here dash is fine, no alias since no need)
#
# Each of these functions takes name of .cc file as 1st argument
# following arguments are optionally dependencies - spelled as above say uno::math::gradient if needed
# or referring to external libraries like Boost::log
#
# For file named "src/x/y/foo" all modules of the form "x::y::foo::..." e.g. sitting under "src/x/y/foo/.."
# are automatically included as dependencies
#
# This is because they are considered to be "submodules" of "x::y::foo"
# Incidentally "x" above is the name of the overall project
#
# This only works if CMakeList for a folder 1st includes subfolders and only then starts using "simple_.." functions
#
# So for a library we generally expect to see "src/x.cc src/x/... include/x.hh tests/x-test.cc tests/x/.." and no other source files
# For an executable we expect "src/main.cc src/x.cc src/x tests/x-test tests/x/..." and no other source files
#
# not working: This file includes "src" and "tests" directories once their -I properties have been set
# since this feature is not working we get a bit of boilerplate for "src/CMakeLists.txt" and "tests/CMakeLists.txt"
# these need to specify "include_directories" correctly
#
# "simple_gtest" adds "gtest::gtest" dependency

include(GoogleTest)

function(simple_module_compute_internals)
    list(POP_FRONT ARGV kind src_file)
    set(deps ${ARGV})

    if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL "${PROJECT_SOURCE_DIR}/${kind}")
        set(mod_prefix "")
        set(prefix_alias "")
    else()
        cmake_path(RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
                BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/${kind}"
                OUTPUT_VARIABLE rel_dir)
        string(REPLACE "/" "-" mod_prefix "${rel_dir}/")
        string(REPLACE "/" "::" prefix_alias "${rel_dir}/")
    endif()

    return(PROPAGATE kind src_file deps mod_prefix prefix_alias)
endfunction()

function(simple_module_get_submodules mod_suffix)
    set(submodules, "")
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${mod_suffix})
        get_property(all DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${mod_suffix} PROPERTY simple_modules_all)
        if(all)
            set(submodules ${all})
        endif()
    endif()
    return(PROPAGATE submodules)
endfunction()

function(simple_module_impl)
    simple_module_compute_internals(${ARGV})
    string(REGEX REPLACE "\\..*" "" mod_suffix ${src_file})
    set(mod "${mod_prefix}${mod_suffix}")

    simple_module_get_submodules(${mod_suffix})
    list(APPEND deps ${submodules})

    message("-- simple ${kind} module: ${prefix_alias}${mod_suffix} [${deps}]")

    add_library(${mod}-dependencies INTERFACE)
    target_link_libraries(${mod}-dependencies INTERFACE ${deps})

    add_library(${mod}-object OBJECT ${src_file})
    target_link_libraries(${mod}-object ${mod}-dependencies)

    # it seems a bit awkward that we're adding both "util.cc.o" and "uno-util-object"
    # as dependencies of "uno" here
    # however otherwise it's not working with cmake 3.31.3
    # if we leave out "uno-util-object" it doesn't build "util.cc.o"
    # if we leave out "util.cc.o" it doesn't link it into the main executable
    # worth asking on cmake discord?..

    add_library(${mod} INTERFACE)
    target_link_libraries(${mod} INTERFACE ${mod}-object ${mod}-dependencies $<TARGET_OBJECTS:${mod}-object>)

    if(DEFINED prefix_alias AND NOT prefix_alias STREQUAL "")
        set(fancy_name ${prefix_alias}${mod_suffix})
        add_library(${fancy_name} ALIAS ${mod})
    else()
        set(fancy_name ${mod})
    endif()

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} APPEND PROPERTY
            simple_modules_all ${fancy_name})
endfunction()

function(simple_module)
    simple_module_impl("src" ${ARGV})
endfunction()

function(simple_test_module)
    simple_module_impl("tests" ${ARGV})
endfunction()

function(simple_gtest)
    simple_module_compute_internals("tests" ${ARGV})
    string(REGEX REPLACE "-test\\..*" "" mod_suffix ${src_file})
    set(mod "${mod_prefix}${mod_suffix}")
    list(APPEND deps gtest::gtest)
    message("-- simple gtest: ${mod}-test [${deps}]")

    add_executable(${mod}-test ${src_file})
    target_link_libraries(${mod}-test ${mod}-dependencies ${deps})

    gtest_discover_tests(${mod}-test)
endfunction()

