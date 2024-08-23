add_library(test INTERFACE test.h)

add_executable(run_tests run_tests.c)
target_link_libraries(run_tests PRIVATE str_view::str_view test)

###########   Add Test Files Below This Point   ###########

# TEST_NAME should be the name of the .c file added to the test folder
# and the macro will add the target, link the libraries, and put the
# the test executable in the tests folder either in debug or release
# depending on the build type. The run_tests program then runs all binaries.

#############  Priority Queue  ##########################

macro(add_depq_test TEST_NAME)
  add_executable(${TEST_NAME} depq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME} PRIVATE
    depqueue 
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

macro(add_fpq_test TEST_NAME)
  add_executable(${TEST_NAME} fpq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME} PRIVATE
    flat_pqueue 
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

macro(add_pq_test TEST_NAME)
  add_executable(${TEST_NAME} pq/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME} PRIVATE
    pqueue 
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

#############  Set  ##########################

macro(add_set_test TEST_NAME)
  add_executable(${TEST_NAME} set/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME} 
    set
    test
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
endmacro()

add_set_test(test_set_construct)
add_set_test(test_set_insert)
add_set_test(test_set_erase)
add_set_test(test_set_iter)

#############  fhash  ##########################

add_library(fhash_util fhash/fhash_util.h fhash/fhash_util.c)
target_link_libraries(fhash_util
  flat_hash
)

macro(add_fhash_test TEST_NAME)
  add_executable(${TEST_NAME} fhash/${TEST_NAME}.c)
  target_link_libraries(${TEST_NAME} PRIVATE
    macros
    flat_hash
    buf
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

################### Performance Testing #################
add_executable(perf perf/perf.c)
target_link_libraries(perf PRIVATE 
  buf
  depqueue 
  flat_pqueue
  pqueue
  random
  str_view::str_view
  cli
)
