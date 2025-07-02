cmake_minimum_required(VERSION 3.15)
project("ccc samples"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Testing the C Container Collection with sample programs."
)

add_custom_target(samples)

add_executable(maze maze.c)
target_include_directories(maze PRIVATE "../util")
target_link_libraries(maze PRIVATE
  cli
  random
  str_view
  ccc
  alloc
)
add_dependencies(samples maze)

add_executable(graph graph.c)
target_include_directories(graph PRIVATE "../util")
target_link_libraries(graph PRIVATE
  cli
  random
  str_view
  ccc
  alloc
)
add_dependencies(samples graph)

add_executable(words words.c)
target_include_directories(words PRIVATE "../util")
target_link_libraries(words PRIVATE
  cli
  str_view
  ccc
  alloc
  str_arena
)
add_dependencies(samples words)

add_executable(ccczip ccczip.c)
target_include_directories(ccczip PRIVATE "../util")
target_link_libraries(ccczip PRIVATE
  cli
  str_view
  ccc
  alloc
  str_arena
)
add_dependencies(samples ccczip)
