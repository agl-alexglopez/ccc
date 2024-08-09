cmake_minimum_required(VERSION 3.23)
project(splay-tree-lib VERSION 1.0 LANGUAGES C)

include(etc/scanners.cmake)

find_package(str_view)

include_directories("${PROJECT_SOURCE_DIR}/ccc")
include_directories("${PROJECT_SOURCE_DIR}/ccc_src")
include_directories("${PROJECT_SOURCE_DIR}/util")
include_directories("${PROJECT_SOURCE_DIR}/samples")
include_directories("${PROJECT_SOURCE_DIR}/tests")

add_subdirectory("${PROJECT_SOURCE_DIR}/ccc")
add_subdirectory("${PROJECT_SOURCE_DIR}/ccc_src")
add_subdirectory("${PROJECT_SOURCE_DIR}/util")
add_subdirectory("${PROJECT_SOURCE_DIR}/samples")
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
