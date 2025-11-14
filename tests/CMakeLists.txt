cmake_minimum_required(VERSION 3.23)
project("ccc tests"
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Testing the C Container Collection (ccc)."
)

####################  Final Target   ##############################
# Create a custom target and each test adds itself as dependency to this target.
# This will allow us to selectively build the test target when needed. This
# cleans up the release distribution and makes Makefile commands easier.
add_custom_target(tests)

add_library(checkers INTERFACE checkers.h)
add_dependencies(tests checkers)


add_executable(run_tests run_tests.c)
target_link_libraries(run_tests PRIVATE checkers string_view)
add_dependencies(tests run_tests)

###########   Add Test Files Below This Point   ###########

# TEST_NAME should be the name of the .c file added to the test folder
# and the macro will add the target, link the libraries, and put the
# the test executable in the tests folder either in debug or release
# depending on the build type. The run_tests program then runs all binaries.

#############  Bit Set ##########################
macro(add_bitset_test TEST_NAME)
  add_executable(${TEST_NAME} bitset/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_bitset_test(test_bitset_construct)
add_bitset_test(test_bitset_insert)
add_bitset_test(test_bitset_erase)
add_bitset_test(test_bitset_test_and_set)

#############  Buffer ##########################
add_library(buffer_utility buffer/buffer_utility.h buffer/buffer_utility.c)
target_link_libraries(buffer_utility
  PRIVATE
    ccc
    checkers
    random
)
add_dependencies(tests buffer_utility)

macro(add_buffer_test TEST_NAME)
  add_executable(${TEST_NAME} buffer/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      buffer_utility
      checkers
      allocate
      random
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_buffer_test(test_buffer_construct)
add_buffer_test(test_buffer_insert)
add_buffer_test(test_buffer_erase)
add_buffer_test(test_buffer_iterator)

#############  Heap Priority Queue  ##########################
add_library(flat_priority_queue_utility flat_priority_queue/flat_priority_queue_utility.h flat_priority_queue/flat_priority_queue_utility.c)
target_link_libraries(flat_priority_queue_utility
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests flat_priority_queue_utility)


macro(add_flat_priority_queue_test TEST_NAME)
  add_executable(${TEST_NAME} flat_priority_queue/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      flat_priority_queue_utility
      ccc
      checkers
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_flat_priority_queue_test(test_flat_priority_queue_construct)
add_flat_priority_queue_test(test_flat_priority_queue_insert)
add_flat_priority_queue_test(test_flat_priority_queue_erase)
add_flat_priority_queue_test(test_flat_priority_queue_update)

#############  Pair Priority Queue  ##########################
add_library(priority_queue_utility priority_queue/priority_queue_utility.h priority_queue/priority_queue_utility.c)
target_link_libraries(priority_queue_utility
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests priority_queue_utility)

macro(add_priority_queue_test TEST_NAME)
  add_executable(${TEST_NAME} priority_queue/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      priority_queue_utility
      ccc
      checkers
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_priority_queue_test(test_priority_queue_construct)
add_priority_queue_test(test_priority_queue_insert)
add_priority_queue_test(test_priority_queue_erase)
add_priority_queue_test(test_priority_queue_update)

#############  Map  ##########################
add_library(adaptive_map_utility adaptive_map/adaptive_map_utility.h adaptive_map/adaptive_map_utility.c)
target_link_libraries(adaptive_map_utility
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests adaptive_map_utility)

macro(add_adaptive_map_test TEST_NAME)
  add_executable(${TEST_NAME} adaptive_map/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      adaptive_map_utility
      ccc
      checkers 
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_adaptive_map_test(test_adaptive_map_construct)
add_adaptive_map_test(test_adaptive_map_insert)
add_adaptive_map_test(test_adaptive_map_erase)
add_adaptive_map_test(test_adaptive_map_iterator)
add_adaptive_map_test(test_adaptive_map_entry)
add_adaptive_map_test(test_adaptive_map_lru)

#############  Handle Map  ##########################
add_library(handle_adaptive_map_utility handle_adaptive_map/handle_adaptive_map_utility.h handle_adaptive_map/handle_adaptive_map_utility.c)
target_link_libraries(handle_adaptive_map_utility
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests handle_adaptive_map_utility)

macro(add_handle_adaptive_map_test TEST_NAME)
  add_executable(${TEST_NAME} handle_adaptive_map/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      handle_adaptive_map_utility
      ccc
      checkers
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_handle_adaptive_map_test(test_handle_adaptive_map_construct)
add_handle_adaptive_map_test(test_handle_adaptive_map_insert)
add_handle_adaptive_map_test(test_handle_adaptive_map_erase)
add_handle_adaptive_map_test(test_handle_adaptive_map_iterator)
add_handle_adaptive_map_test(test_handle_adaptive_map_entry)
add_handle_adaptive_map_test(test_handle_adaptive_map_lru)

#############  Realtime Map  ##########################
add_library(bounded_map_utility bounded_map/bounded_map_utility.h bounded_map/bounded_map_utility.c)
target_link_libraries(bounded_map_utility
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests bounded_map_utility)

macro(add_bounded_map_test TEST_NAME)
  add_executable(${TEST_NAME} bounded_map/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      bounded_map_utility
      checkers
      ccc
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_bounded_map_test(test_bounded_map_construct)
add_bounded_map_test(test_bounded_map_insert)
add_bounded_map_test(test_bounded_map_erase)
add_bounded_map_test(test_bounded_map_iterator)
add_bounded_map_test(test_bounded_map_entry)
add_bounded_map_test(test_bounded_map_lru)

#############  Handle Realtime Map  ##########################
add_library(handle_bounded_map_utility handle_bounded_map/handle_bounded_map_utility.h handle_bounded_map/handle_bounded_map_utility.c)
target_link_libraries(handle_bounded_map_utility
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests handle_bounded_map_utility)

macro(add_handle_bounded_map_test TEST_NAME)
  add_executable(${TEST_NAME} handle_bounded_map/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      handle_bounded_map_utility
      ccc
      checkers
      allocate
      random
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_handle_bounded_map_test(test_handle_bounded_map_construct)
add_handle_bounded_map_test(test_handle_bounded_map_insert)
add_handle_bounded_map_test(test_handle_bounded_map_erase)
add_handle_bounded_map_test(test_handle_bounded_map_iterator)
add_handle_bounded_map_test(test_handle_bounded_map_handle)
add_handle_bounded_map_test(test_handle_bounded_map_lru)

#############  Flat Hash Map ##########################

add_library(flat_hash_map_utility flat_hash_map/flat_hash_map_utility.h flat_hash_map/flat_hash_map_utility.c)
target_link_libraries(flat_hash_map_utility
    ccc
)
add_dependencies(tests flat_hash_map_utility)

macro(add_flat_hash_map_test TEST_NAME)
  add_executable(${TEST_NAME} flat_hash_map/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      checkers
      flat_hash_map_utility
      random
      ccc
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_flat_hash_map_test(test_flat_hash_map_construct)
add_flat_hash_map_test(test_flat_hash_map_insert)
add_flat_hash_map_test(test_flat_hash_map_erase)
add_flat_hash_map_test(test_flat_hash_map_lru)
add_flat_hash_map_test(test_flat_hash_map_entry)
add_flat_hash_map_test(test_flat_hash_map_iterator)

#############  Doubly Linked List ##########################

add_library(doubly_linked_list_utility doubly_linked_list/doubly_linked_list_utility.h doubly_linked_list/doubly_linked_list_utility.c)
target_link_libraries(doubly_linked_list_utility
  ccc
)
add_dependencies(tests doubly_linked_list_utility)

macro(add_doubly_linked_list_test TEST_NAME)
  add_executable(${TEST_NAME} doubly_linked_list/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers
      doubly_linked_list_utility
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_doubly_linked_list_test(test_doubly_linked_list_construct)
add_doubly_linked_list_test(test_doubly_linked_list_insert)
add_doubly_linked_list_test(test_doubly_linked_list_erase)

#############  Singly Linked List ##########################

add_library(singly_linked_list_utility singly_linked_list/singly_linked_list_utility.h singly_linked_list/singly_linked_list_utility.c)
target_link_libraries(singly_linked_list_utility
  ccc
)
add_dependencies(tests singly_linked_list_utility)

macro(add_singly_linked_list_test TEST_NAME)
  add_executable(${TEST_NAME} singly_linked_list/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers 
      singly_linked_list_utility
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_singly_linked_list_test(test_singly_linked_list_construct)
add_singly_linked_list_test(test_singly_linked_list_insert)
add_singly_linked_list_test(test_singly_linked_list_erase)

#############  Flat Double Ended Queue ##########################

add_library(flat_double_ended_queue_utility flat_double_ended_queue/flat_double_ended_queue_utility.h flat_double_ended_queue/flat_double_ended_queue_utility.c)
target_link_libraries(flat_double_ended_queue_utility
  ccc
)
add_dependencies(tests flat_double_ended_queue_utility)

macro(add_flat_double_ended_queue_test TEST_NAME)
  add_executable(${TEST_NAME} flat_double_ended_queue/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers 
      flat_double_ended_queue_utility
      allocate
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_flat_double_ended_queue_test(test_flat_double_ended_queue_construct)
add_flat_double_ended_queue_test(test_flat_double_ended_queue_insert)
add_flat_double_ended_queue_test(test_flat_double_ended_queue_erase)

