cmake_minimum_required(VERSION 3.23)
project("ccc tests"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Testing the C Container Collection (ccc)."
)

find_package(str_view)

add_library(checkers INTERFACE checkers.h)
include_directories("../ccc")
include_directories("../util")

add_executable(run_tests run_tests.c)
target_link_libraries(run_tests PRIVATE checkers str_view::str_view)

###########   Add Test Files Below This Point   ###########

# TEST_NAME should be the name of the .c file added to the test folder
# and the macro will add the target, link the libraries, and put the
# the test executable in the tests folder either in debug or release
# depending on the build type. The run_tests program then runs all binaries.

#############  Ordered Multimap ##########################
add_library(ommap_util ommap/ommap_util.h ommap/ommap_util.c)
target_link_libraries(ommap_util
  PRIVATE
    ordered_multimap
    checkers
    traits
)

macro(add_ommap_test TEST_NAME)
  add_executable(${TEST_NAME} ommap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      ordered_multimap 
      ommap_util
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_ommap_test(test_ommap_construct)
add_ommap_test(test_ommap_insert)
add_ommap_test(test_ommap_erase)
add_ommap_test(test_ommap_iter)

#############  Heap Priority Queue  ##########################
add_library(fpq_util fpq/fpq_util.h fpq/fpq_util.c)
target_link_libraries(fpq_util
  PRIVATE
    traits
    flat_priority_queue
    checkers
)

macro(add_fpq_test TEST_NAME)
  add_executable(${TEST_NAME} fpq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      fpq_util
      flat_priority_queue 
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_fpq_test(test_fpq_construct)
add_fpq_test(test_fpq_insert)
add_fpq_test(test_fpq_erase)
add_fpq_test(test_fpq_update)

#############  Pair Priority Queue  ##########################
add_library(pq_util pq/pq_util.h pq/pq_util.c)
target_link_libraries(pq_util
  PRIVATE
    traits
    priority_queue
    checkers
)

macro(add_pq_test TEST_NAME)
  add_executable(${TEST_NAME} pq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      pq_util
      priority_queue 
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_pq_test(test_pq_construct)
add_pq_test(test_pq_insert)
add_pq_test(test_pq_erase)
add_pq_test(test_pq_update)

#############  Map  ##########################
add_library(omap_util omap/omap_util.h omap/omap_util.c)
target_link_libraries(omap_util
  PRIVATE
    traits
    ordered_map
    checkers
)

macro(add_omap_test TEST_NAME)
  add_executable(${TEST_NAME} omap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      omap_util
      ordered_map
      checkers 
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_omap_test(test_omap_construct)
add_omap_test(test_omap_insert)
add_omap_test(test_omap_erase)
add_omap_test(test_omap_iter)

#############  Flat Map  ##########################
add_library(fomap_util fomap/fomap_util.h fomap/fomap_util.c)
target_link_libraries(fomap_util
  PRIVATE
    traits
    flat_ordered_map
    checkers
)

macro(add_fomap_test TEST_NAME)
  add_executable(${TEST_NAME} fomap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      fomap_util
      flat_ordered_map
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_fomap_test(test_fomap_construct)
add_fomap_test(test_fomap_insert)
add_fomap_test(test_fomap_erase)
add_fomap_test(test_fomap_iter)

#############  Realtime Map  ##########################
add_library(romap_util romap/romap_util.h romap/romap_util.c)
target_link_libraries(romap_util
  PRIVATE
    traits
    realtime_ordered_map
    checkers
)

macro(add_romap_test TEST_NAME)
  add_executable(${TEST_NAME} romap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      romap_util
      ordered_map
      checkers
      traits
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_romap_test(test_romap_construct)
add_romap_test(test_romap_insert)
add_romap_test(test_romap_erase)
add_romap_test(test_romap_iter)

#############  Flat Realtime Map  ##########################
add_library(fromap_util fromap/fromap_util.h fromap/fromap_util.c)
target_link_libraries(fromap_util
  PRIVATE
    traits
    flat_realtime_ordered_map
    checkers
)

macro(add_fromap_test TEST_NAME)
  add_executable(${TEST_NAME} fromap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      fromap_util
      ordered_map
      checkers
      traits
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_fromap_test(test_fromap_construct)
add_fromap_test(test_fromap_insert)
add_fromap_test(test_fromap_erase)
add_fromap_test(test_fromap_iter)

#############  Flat Hash Map ##########################

add_library(fhmap_util fhmap/fhmap_util.h fhmap/fhmap_util.c)
target_link_libraries(fhmap_util
  flat_hash_map
)

macro(add_fhmap_test TEST_NAME)
  add_executable(${TEST_NAME} fhmap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      flat_hash_map
      doubly_linked_list
      checkers
      fhmap_util
      traits
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_fhmap_test(test_fhmap_construct)
add_fhmap_test(test_fhmap_insert)
add_fhmap_test(test_fhmap_erase)
add_fhmap_test(test_fhmap_lru)

#############  Doubly Linked List ##########################

add_library(dll_util dll/dll_util.h dll/dll_util.c)
target_link_libraries(dll_util
  doubly_linked_list
)

macro(add_dll_test TEST_NAME)
  add_executable(${TEST_NAME} dll/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      doubly_linked_list
      checkers
      dll_util
      traits
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_dll_test(test_dll_construct)
add_dll_test(test_dll_insert)
add_dll_test(test_dll_erase)

#############  Singly Linked List ##########################

add_library(sll_util sll/sll_util.h sll/sll_util.c)
target_link_libraries(sll_util
  singly_linked_list
)

macro(add_sll_test TEST_NAME)
  add_executable(${TEST_NAME} sll/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      singly_linked_list
      checkers 
      sll_util
      traits
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_sll_test(test_sll_construct)
add_sll_test(test_sll_insert)
add_sll_test(test_sll_erase)

#############  Flat Double Ended Queue ##########################

add_library(fdeq_util fdeq/fdeq_util.h fdeq/fdeq_util.c)
target_link_libraries(fdeq_util
  flat_double_ended_queue
)

macro(add_fdeq_test TEST_NAME)
  add_executable(${TEST_NAME} fdeq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      flat_double_ended_queue
      checkers 
      fdeq_util
      traits
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_fdeq_test(test_fdeq_construct)
add_fdeq_test(test_fdeq_insert)
add_fdeq_test(test_fdeq_erase)

################### Performance Testing #################
add_executable(perf perf/perf.c)
target_link_libraries(perf 
  PRIVATE 
    buffer
    ordered_multimap 
    flat_priority_queue
    priority_queue
    random
    str_view::str_view
    cli
    traits
    alloc
)

# Create a custom target that will depend on all targets in the subdirectory
add_custom_target(tests)
# Function to add dependencies on all targets from the subdirectory
get_property(test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS test_targets)
    add_dependencies(tests ${target})
endforeach()
