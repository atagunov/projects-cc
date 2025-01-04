# This project is using a "pseudo-module" facility
# actual C++20 modules are perceived as not being mature enough for prod use at the moment
#
# "simple_module" function facilitates building this "pseudo-module" hierarchy
# Each src/**/*.cc file is considered a "simple_module"
# So we invoke this function once for each src/**/*.cc file
#
# 1st argument is short name (no directory prefix) of the .cc
# The following arguments are "modules" e.g. other .cc files it depends once
#
# What this function actually does is it invokes add_library( .. OBJECT ..) twice
# 1st it creates a "library" which contains only dependencies of our "simple module" file - other .cc files
# 2nd it creates a "library" which contains all of the above plus the .cc file itself
#
# So we get for example "uno-util-dependencies" and "uno-util" OBJECT libraries
#
# The former is used when compiling -test.cc for the source file
# We need say it to be separate from "uno-util"
# because "uno/util-test.cc" actually _includes_ "uno/util.cc" in order to do white-box testing
#
# The bigger "uno-util" is used when higher-level modules like "uno" are declared
# Each higher-level module includes .o files for all subordinate modules into its targets
#
# Additionally this function adds adds global-includes as a private dependency to both of
# the "library" targets it creates so that we use correct -I when compiling

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
        add_library(${mod}-dependencies OBJECT ${empty_file})
    endif()

    add_library(${mod} OBJECT ${src_file})
    target_link_libraries(${mod} PRIVATE global-includes ${mod}-dependencies)
endfunction()
