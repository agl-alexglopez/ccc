cmake_minimum_required(VERSION 3.23)
project("ccc util"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Utilities for testing and samples."
)

find_package(str_view)

add_library(random random.h random.c)
add_library(cli cli.h cli.c)
target_link_libraries(cli PUBLIC
  str_view::str_view
)
add_library(alloc alloc.h alloc.c)

add_custom_target(util)
# Function to add dependencies on all targets from the subdirectory
# Get the list of all targets
get_property(util_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS util_targets)
    add_dependencies(util ${target})
endforeach()
