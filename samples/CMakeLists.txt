cmake_minimum_required(VERSION 3.15)
project("ccc samples"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Testing the C Container Collection with sample programs."
)

find_package(str_view)
include_directories("../ccc")
include_directories("../util")

add_executable(maze maze.c)
target_link_libraries(maze PRIVATE
  cli
  random
  str_view::str_view
  ordered_map
  double_ended_priority_queue
  traits
)

add_executable(graph graph.c)
target_link_libraries(graph PRIVATE
  cli
  random
  str_view::str_view
  flat_hash_map
  flat_double_ended_queue
  priority_queue
  flat_priority_queue
  traits
)

add_executable(words words.c)
target_link_libraries(words PRIVATE
  cli
  str_view::str_view
  realtime_ordered_map
  flat_realtime_ordered_map
  ordered_map
  flat_ordered_map
  flat_priority_queue
  types
  traits
)

add_custom_target(samples)
# Function to add dependencies on all targets from the subdirectory
# Get the list of all targets
get_property(sample_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS sample_targets)
    add_dependencies(samples ${target})
endforeach()
