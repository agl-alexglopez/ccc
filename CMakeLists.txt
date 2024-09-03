cmake_minimum_required(VERSION 3.23)
project("ccc" 
  VERSION 0.1 
  LANGUAGES C
  DESCRIPTION "The C Container Collection (ccc). A collection of containers written in C."
)

set(namespace "ccc")
find_package(str_view)

include_directories("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/ccc")
add_subdirectory("${PROJECT_SOURCE_DIR}/ccc")

# For the sake of vcpkg we will make sure cloning and building from
# main will only build the minimal files in the all target. I can't
# protect vcpkg from needlessly downloading 
if (EXISTS "${PROJECT_SOURCE_DIR}/samples")
    include_directories("${PROJECT_SOURCE_DIR}/samples")
    add_subdirectory("${PROJECT_SOURCE_DIR}/samples" EXCLUDE_FROM_ALL)
endif()
if (EXISTS "${PROJECT_SOURCE_DIR}/tests")
    include_directories("${PROJECT_SOURCE_DIR}/tests")
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests" EXCLUDE_FROM_ALL)
endif()
if (EXISTS "${PROJECT_SOURCE_DIR}/tests" AND EXISTS "${PROJECT_SOURCE_DIR}/samples")
    include(etc/scanners.cmake)
    include_directories("${PROJECT_SOURCE_DIR}/util")
    add_subdirectory("${PROJECT_SOURCE_DIR}/util" EXCLUDE_FROM_ALL)
endif()

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "This project has a top-level one called [${CMAKE_PROJECT_NAME}]")
else()
    message(STATUS "This project is a top-level one")
endif()


option(BUILD_SHARED_LIBS "Building str_view as a shared library is available, but OFF by default." OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/debug/bin)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()


# where to find our CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#message(STATUS "CMake module path: ${CMAKE_MODULE_PATH}")
include(Installing)
