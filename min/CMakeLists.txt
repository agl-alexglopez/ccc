cmake_minimum_required(VERSION 3.23)
project("min ccc" 
  VERSION 1
  LANGUAGES C
  DESCRIPTION "Minimum Reproducible C Container Collection Usage."
)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(ccc HINTS "../ccc-dev/install")

add_executable(min min.c)
target_link_libraries(min ccc::ccc)
