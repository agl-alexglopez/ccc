add_library(test INTERFACE test.h)

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
)

macro(add_depq_test TEST_NAME)
  add_executable(${TEST_NAME} depq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
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
    flat_priority_queue
    test
)

macro(add_fpq_test TEST_NAME)
  add_executable(${TEST_NAME} fpq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
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
    priority_queue
    test
)

macro(add_pq_test TEST_NAME)
  add_executable(${TEST_NAME} pq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
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
    map
    test
)

macro(add_map_test TEST_NAME)
  add_executable(${TEST_NAME} map/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      map_util
      map
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

#############  fhash  ##########################

add_library(fhash_util fhash/fhash_util.h fhash/fhash_util.c)
target_link_libraries(fhash_util
  flat_hash
)

macro(add_fhash_test TEST_NAME)
  add_executable(${TEST_NAME} fhash/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME}
    PRIVATE
      flat_hash
      test
      fhash_util
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
)
