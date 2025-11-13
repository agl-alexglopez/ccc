cmake_minimum_required(VERSION 3.23)
project("ccc utility"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Utilities for testing and samples."
)
add_custom_target(utility)

add_library(string_view string_view/string_view.h string_view/string_view.c)
add_dependencies(utility string_view)

add_library(random random.h random.c)
add_dependencies(utility random)

add_library(cli cli.h cli.c)
target_link_libraries(cli PUBLIC
  string_view
)
add_dependencies(utility cli)

add_library(allocate allocate.h allocate.c)
target_link_libraries(allocate ccc)
add_dependencies(utility allocate)

add_library(string_arena string_arena.h string_arena.c)
add_dependencies(utility string_arena)
