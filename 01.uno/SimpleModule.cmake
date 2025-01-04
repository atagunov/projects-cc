# This project is using a "pseudo-module" facility
# actual C++20 modules are perceived as not being mature enough for prod use at the moment
#
# simple_module function facilitates building this "pseudo-module" hierarchy
# each src/**/*.cc file is considered a "simple_module"
# so we invoke this function one for each src/**/*.cc file
#
# 1st argument is short name (no directory prefix) of the .cc
# following arguments are "modules" e.g. other .cc files it depends once
#
# what this function actually does is it invokes add_library( .. OBJECT ..) twice
# 1st it creates a "library" which contains only dependencies of our "simple module" dependencies - other .cc files
# 2nd it create a "library" which also contains the source file itself
#
# the former is used when compiling -test.cc for the source file
# we need it to be separate because util-test.cc actually _includes_ util.cc in order to do white-box testing
#
# the later is used when other .cc files dependent on this one are built
#
# additionally this function adds adds global-includes as a private dependency to both of the "library" targets it creates

function(simple_module)
    if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL "${PROJECT_SOURCE_DIR}/src")
        set(mod_prefix "")
    else()
        cmake_path(RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
                BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/src"
                OUTPUT_VARIABLE rel_dir)
        string(REPLACE "/" "-" mod_prefix "${rel_dir}/")
    endif()

    list(POP_FRONT ARGV src_file)
    string(REGEX REPLACE "\\..*" "" mod_suffix ${src_file})
    set(mod "${mod_prefix}${mod_suffix}")

    if(ARGV)
        message("-- simple module: ${mod} [${ARGV}]")
        list(TRANSFORM ARGV REPLACE "(.+)" "$<TARGET_OBJECTS:\\1>")
        add_library(${mod}-dependencies OBJECT ${ARGV})
        target_link_libraries(${mod}-dependencies PRIVATE global-includes)
    else()
        message("-- simple module: ${mod}")
        # it seems there is really no way to have empty OBJECT libraries presently in cmake
        set(empty_file "${CMAKE_CURRENT_BINARY_DIR}/empty.cc")
        if(NOT EXISTS ${empty_file})
            file(TOUCH ${empty_file})
        endif()
        add_library(${mod}-dependencies OBJECT "${CMAKE_CURRENT_BINARY_DIR}/empty.cc")
    endif()

    add_library(${mod} OBJECT ${src_file})
    target_link_libraries(${mod} PRIVATE global-includes ${mod}-dependencies)
endfunction()
