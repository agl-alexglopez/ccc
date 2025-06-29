cmake_minimum_required(VERSION 3.23)
project("ccc util"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Utilities for testing and samples."
)
add_custom_target(util)

add_library(str_view str_view.h str_view.c)
add_dependencies(util str_view)

add_library(random random.h random.c)
add_dependencies(util random)

add_library(cli cli.h cli.c)
target_link_libraries(cli PUBLIC
  str_view
)
add_dependencies(util cli)

add_library(alloc alloc.h alloc.c)
add_dependencies(util alloc)

add_library(str_arena str_arena.h str_arena.c)
add_dependencies(util str_arena)
