cmake_minimum_required(VERSION 3.15)
project("ccc samples"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Testing the C Container Collection with sample programs."
)

add_executable(maze maze.c)
target_include_directories(maze PRIVATE "../util")
target_link_libraries(maze PRIVATE
  cli
  random
  str_view
  ccc
  alloc
)

add_executable(graph graph.c)
target_include_directories(graph PRIVATE "../util")
target_link_libraries(graph PRIVATE
  cli
  random
  str_view
  ccc
  alloc
)

add_executable(words words.c)
target_include_directories(words PRIVATE "../util")
target_link_libraries(words PRIVATE
  cli
  str_view
  ccc
  alloc
)

add_custom_target(samples)
# Function to add dependencies on all targets from the subdirectory
# Get the list of all targets
get_property(sample_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS sample_targets)
    add_dependencies(samples ${target})
endforeach()
