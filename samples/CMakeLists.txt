cmake_minimum_required(VERSION 3.15)
project("ccc samples"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Testing the C Container Collection with sample programs."
)

find_package(str_view)
find_package(ccc HINTS "../intall")
include_directories("../util")

add_executable(maze maze.c)
target_link_libraries(maze PRIVATE
  cli
  random
  str_view::str_view
  ccc
)

add_executable(graph graph.c)
target_link_libraries(graph PRIVATE
  cli
  random
  str_view::str_view
  ccc
)

add_executable(words words.c)
target_link_libraries(words PRIVATE
  cli
  str_view::str_view
  ccc
)

add_custom_target(samples)
# Function to add dependencies on all targets from the subdirectory
# Get the list of all targets
get_property(sample_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS sample_targets)
    add_dependencies(samples ${target})
endforeach()
