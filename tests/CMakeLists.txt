cmake_minimum_required(VERSION 3.23)
project("ccc tests"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Testing the C Container Collection (ccc)."
)

find_package(str_view)

add_library(test INTERFACE test.h)
include_directories("../ccc")
include_directories("../util")

add_executable(run_tests run_tests.c)
target_link_libraries(run_tests PRIVATE str_view::str_view test)

###########   Add Test Files Below This Point   ###########

# TEST_NAME should be the name of the .c file added to the test folder
# and the macro will add the target, link the libraries, and put the
# the test executable in the tests folder either in debug or release
# depending on the build type. The run_tests program then runs all binaries.

#############  Priority Queue  ##########################
add_library(depq_util depq/depq_util.h depq/depq_util.c)
target_link_libraries(depq_util
  PRIVATE
    double_ended_priority_queue
    test
    traits
)

macro(add_depq_test TEST_NAME)
  add_executable(${TEST_NAME} depq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      double_ended_priority_queue 
      depq_util
      test
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_depq_test(test_depq_construct)
add_depq_test(test_depq_insert)
add_depq_test(test_depq_erase)
add_depq_test(test_depq_iter)

#############  Heap Priority Queue  ##########################
add_library(fpq_util fpq/fpq_util.h fpq/fpq_util.c)
target_link_libraries(fpq_util
  PRIVATE
    traits
    flat_priority_queue
    test
)

macro(add_fpq_test TEST_NAME)
  add_executable(${TEST_NAME} fpq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      fpq_util
      flat_priority_queue 
      test
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
    test
)

macro(add_pq_test TEST_NAME)
  add_executable(${TEST_NAME} pq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      pq_util
      priority_queue 
      test
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
add_library(map_util map/map_util.h map/map_util.c)
target_link_libraries(map_util
  PRIVATE
    traits
    ordered_map
    test
)

macro(add_map_test TEST_NAME)
  add_executable(${TEST_NAME} map/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      map_util
      ordered_map
      test
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_map_test(test_map_construct)
add_map_test(test_map_insert)
add_map_test(test_map_erase)
add_map_test(test_map_iter)

#############  Flat Map  ##########################
add_library(fmap_util fmap/fmap_util.h fmap/fmap_util.c)
target_link_libraries(fmap_util
  PRIVATE
    traits
    flat_ordered_map
    test
)

macro(add_fmap_test TEST_NAME)
  add_executable(${TEST_NAME} fmap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      traits
      fmap_util
      flat_ordered_map
      test
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_fmap_test(test_fmap_construct)
add_fmap_test(test_fmap_insert)
add_fmap_test(test_fmap_erase)
add_fmap_test(test_fmap_iter)

#############  Realtime Map  ##########################
add_library(rtomap_util rtomap/rtomap_util.h rtomap/rtomap_util.c)
target_link_libraries(rtomap_util
  PRIVATE
    traits
    realtime_ordered_map
    test
)

macro(add_rtomap_test TEST_NAME)
  add_executable(${TEST_NAME} rtomap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      rtomap_util
      ordered_map
      test
      traits
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_rtomap_test(test_rtomap_construct)
add_rtomap_test(test_rtomap_insert)
add_rtomap_test(test_rtomap_erase)
add_rtomap_test(test_rtomap_iter)

#############  Flat Realtime Map  ##########################
add_library(frtomap_util frtomap/frtomap_util.h frtomap/frtomap_util.c)
target_link_libraries(frtomap_util
  PRIVATE
    traits
    flat_realtime_ordered_map
    test
)

macro(add_frtomap_test TEST_NAME)
  add_executable(${TEST_NAME} frtomap/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      frtomap_util
      ordered_map
      test
      traits
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_frtomap_test(test_frtomap_construct)
add_frtomap_test(test_frtomap_insert)
add_frtomap_test(test_frtomap_erase)
add_frtomap_test(test_frtomap_iter)

#############  Flat Hash Map ##########################

add_library(fhash_util fhash/fhash_util.h fhash/fhash_util.c)
target_link_libraries(fhash_util
  flat_hash_map
)

macro(add_fhash_test TEST_NAME)
  add_executable(${TEST_NAME} fhash/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      flat_hash_map
      doubly_linked_list
      test
      fhash_util
      traits
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_fhash_test(test_fhash_construct)
add_fhash_test(test_fhash_insert)
add_fhash_test(test_fhash_erase)
add_fhash_test(test_fhash_lru)

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
      test
      dll_util
      traits
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
      test
      sll_util
      traits
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
      test
      fdeq_util
      traits
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
    double_ended_priority_queue 
    flat_priority_queue
    priority_queue
    random
    str_view::str_view
    cli
    traits
)

# Create a custom target that will depend on all targets in the subdirectory
add_custom_target(tests)
# Function to add dependencies on all targets from the subdirectory
get_property(test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS test_targets)
    add_dependencies(tests ${target})
endforeach()
