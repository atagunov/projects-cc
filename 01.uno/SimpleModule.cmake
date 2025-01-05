# This project is using a "pseudo-module" facility
# Actual C++20 modules are perceived as not being mature enough for prod use at the moment
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
# So we get for example "uno-util-dependencies" and "uno-util" cmake targets
# Each is just a collection of .o files
#
# We use "uno-util-dependencies" when compiling "uno/util-test.cc"
# We do it this way because "uno/util-test.cc" actually _includes_ "uno/util.cc" in order to do white-box testing
#
# The bigger "uno-util" is used when higher-level "modules" like "uno" are composed
# Each higher-level module includes .o files for all subordinate modules
# For example "uno" target includes both "uno.o" and "uno/util.o" as well as any other .o files which "util.o" may need
#
# Additionally "simple_module" function adds adds global-includes as a private dependency to both of
# the "library" targets it creates so that we use correct -I when compiling
#
# For an executable "global-includes" just specifies to use "src" folder for "-I"
# If we were building a library we could have included both "src" (for our private headers)
# and a separate "include" folder which would then contain the public API of our library

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
        list(TRANSFORM ARGV REPLACE "(.+)" "$<TARGET_OBJECTS:\\1>" OUTPUT_VARIABLE dep_objects)
    else()
        message("-- simple module: ${mod}")
        set(dep_objects "$<TARGET_OBJECTS:simple-module-empty-object-library>")
    endif()

    add_library(${mod}-dependencies OBJECT ${dep_objects})

    add_library(${mod} OBJECT ${src_file})
    target_link_libraries(${mod} PRIVATE global-includes)
    target_link_libraries(${mod} INTERFACE ${mod}-dependencies)
endfunction()

add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/empty.cc"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/empty.cc")

add_library(simple-module-empty-object-library OBJECT "${CMAKE_BINARY_DIR}/empty.cc")
