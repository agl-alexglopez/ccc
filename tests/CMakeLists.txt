add_library(test INTERFACE test.h)
add_library(str_view str_view.h str_view.c)

add_executable(run_tests run_tests.c)
target_link_libraries(run_tests PRIVATE str_view test)

###########   Add Test Files Below This Point   ###########

# TEST_NAME should be the name of the .c file added to the test folder
# and the macro will add the target, link the libraries, and put the
# the test executable in the tests folder either in debug or release
# depending on the build type. The run_tests program then runs all binaries.
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
add_pq_test(test_pq_iter)

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


