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

function(simple_module_compute_internals)
    list(POP_FRONT ARGV kind src_file)

    if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL "${PROJECT_SOURCE_DIR}/${kind}")
        set(mod_prefix "")
    else()
        cmake_path(RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
                BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/${kind}"
                OUTPUT_VARIABLE rel_dir)
        string(REPLACE "/" "-" mod_prefix "${rel_dir}/")
    endif()

    set(print_deps ${ARGV})
    set(raw_deps ${ARGV})
    set(obj_deps ${ARGV})
    list(FILTER raw_deps INCLUDE REGEX "::")
    list(FILTER obj_deps EXCLUDE REGEX "::")

    if(obj_deps)
        list(TRANSFORM obj_deps REPLACE "(^.+$)" "$<TARGET_OBJECTS:\\1>")
    endif()

    return(PROPAGATE mod_prefix src_file print_deps obj_deps raw_deps)
endfunction()

function(simple_module)
    simple_module_compute_internals("src" ${ARGV})

    #message("mod_prefix=${mod_prefix} src_file=${src_file} raw_deps=${raw_deps} obj_deps=${obj_deps}")
    string(REGEX REPLACE "\\..*" "" mod_suffix ${src_file})
    set(mod "${mod_prefix}${mod_suffix}")
    message("-- simple module: ${mod} [${print_deps}]")

    if (obj_deps)
        message(">>> add_library(${mod}-dependencies OBJECT ${obj_deps})")
        add_library(${mod}-dependencies OBJECT ${obj_deps})
    else()
        # it seems there is really no way to have empty OBJECT libraries presently in cmake
        add_library(${mod}-dependencies OBJECT "$<TARGET_OBJECTS:simple-module-empty-object-library>")
    endif()

    if(raw_deps)
        #message("${mod}-dependencies includes ${raw_deps}")
        target_link_libraries(${mod}-dependencies PUBLIC ${raw_deps})
    endif()

    message(">>> add_library(${mod} OBJECT ${src_file})")
    add_library(${mod} OBJECT ${src_file})
    target_link_libraries(${mod} PRIVATE global-includes)
    message(">>> target_link_libraries(${mod} PUBLIC ${mod}-dependencies)")
    target_link_libraries(${mod} PUBLIC ${mod}-dependencies)
endfunction()

function(simple_module_test)
    simple_module_compute_internals("test" ${ARGV})
    string(REGEX REPLACE "-test\\..*" "" mod_suffix ${src_file})
    set(mod "${mod_prefix}${mod_suffix}")

    #message("mod=${mod} mod_prefix=${mod_prefix} src_file=${src_file} raw_deps=${raw_deps} obj_deps=${obj_deps}")
    message("-- simple module test: ${mod}-test [${print_deps}]")

    add_executable("${mod}-test" ${src_file})
    target_link_libraries(${mod}-test global-includes)

    set(mod_dependencies "${mod}-dependencies")

    # not clear why a test module would depend on other modules directly.. but let's have it for completeness
    target_link_libraries("${mod}-test" "$<TARGET_OBJECTS:${mod_dependencies}>" ${obj_deps} ${raw_deps})

    if(raw_deps)
        target_link_libraries("${mod}-test" ${raw_deps})
    endif()
endfunction()

add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/empty.cc"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/empty.cc")

add_library(simple-module-empty-object-library OBJECT "${CMAKE_BINARY_DIR}/empty.cc")
