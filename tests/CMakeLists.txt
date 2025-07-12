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
target_include_directories(run_tests PRIVATE "../util")
target_link_libraries(run_tests PRIVATE checkers str_view)
add_dependencies(tests run_tests)

###########   Add Test Files Below This Point   ###########

# TEST_NAME should be the name of the .c file added to the test folder
# and the macro will add the target, link the libraries, and put the
# the test executable in the tests folder either in debug or release
# depending on the build type. The run_tests program then runs all binaries.

#############  Bit Set ##########################
macro(add_bs_test TEST_NAME)
  add_executable(${TEST_NAME} bs/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

# Add tests below here by the name of the c file without the .c suffix
add_bs_test(test_bs_construct)
add_bs_test(test_bs_insert)
add_bs_test(test_bs_erase)
add_bs_test(test_bs_test_and_set)

#############  Buffer ##########################
add_library(buf_util buf/buf_util.h buf/buf_util.c)
target_include_directories(buf_util PRIVATE "../util")
target_link_libraries(buf_util
  PRIVATE
    ccc
    checkers
    random
)
add_dependencies(tests buf_util)

macro(add_buf_test TEST_NAME)
  add_executable(${TEST_NAME} buf/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      buf_util
      checkers
      alloc
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
add_buf_test(test_buf_construct)
add_buf_test(test_buf_insert)
add_buf_test(test_buf_erase)
add_buf_test(test_buf_iter)

#############  Heap Priority Queue  ##########################
add_library(fpq_util fpq/fpq_util.h fpq/fpq_util.c)
target_link_libraries(fpq_util
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests fpq_util)


macro(add_fpq_test TEST_NAME)
  add_executable(${TEST_NAME} fpq/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      fpq_util
      ccc
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
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
    ccc
    checkers
)
add_dependencies(tests pq_util)

macro(add_pq_test TEST_NAME)
  add_executable(${TEST_NAME} pq/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      pq_util
      ccc
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
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
    ccc
    checkers
)
add_dependencies(tests omap_util)

macro(add_omap_test TEST_NAME)
  add_executable(${TEST_NAME} omap/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      omap_util
      ccc
      checkers 
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_omap_test(test_omap_construct)
add_omap_test(test_omap_insert)
add_omap_test(test_omap_erase)
add_omap_test(test_omap_iter)
add_omap_test(test_omap_entry)
add_omap_test(test_omap_lru)

#############  Handle Map  ##########################
add_library(homap_util homap/homap_util.h homap/homap_util.c)
target_link_libraries(homap_util
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests homap_util)

macro(add_homap_test TEST_NAME)
  add_executable(${TEST_NAME} homap/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      homap_util
      ccc
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_homap_test(test_homap_construct)
add_homap_test(test_homap_insert)
add_homap_test(test_homap_erase)
add_homap_test(test_homap_iter)
add_homap_test(test_homap_entry)
add_homap_test(test_homap_lru)

#############  Realtime Map  ##########################
add_library(romap_util romap/romap_util.h romap/romap_util.c)
target_link_libraries(romap_util
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests romap_util)

macro(add_romap_test TEST_NAME)
  add_executable(${TEST_NAME} romap/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      romap_util
      checkers
      ccc
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_romap_test(test_romap_construct)
add_romap_test(test_romap_insert)
add_romap_test(test_romap_erase)
add_romap_test(test_romap_iter)
add_romap_test(test_romap_entry)
add_romap_test(test_romap_lru)

#############  Handle Realtime Map  ##########################
add_library(hromap_util hromap/hromap_util.h hromap/hromap_util.c)
target_link_libraries(hromap_util
  PRIVATE
    ccc
    checkers
)
add_dependencies(tests hromap_util)

macro(add_hromap_test TEST_NAME)
  add_executable(${TEST_NAME} hromap/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      hromap_util
      ccc
      checkers
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_hromap_test(test_hromap_construct)
add_hromap_test(test_hromap_insert)
add_hromap_test(test_hromap_erase)
add_hromap_test(test_hromap_iter)
add_hromap_test(test_hromap_handle)
add_hromap_test(test_hromap_lru)

#############  Flat Hash Map ##########################

add_library(fhmap_util fhmap/fhmap_util.h fhmap/fhmap_util.c)
target_link_libraries(fhmap_util
    ccc
)
add_dependencies(tests fhmap_util)

macro(add_fhmap_test TEST_NAME)
  add_executable(${TEST_NAME} fhmap/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      checkers
      fhmap_util
      random
      ccc
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_fhmap_test(test_fhmap_construct)
add_fhmap_test(test_fhmap_insert)
add_fhmap_test(test_fhmap_erase)
add_fhmap_test(test_fhmap_lru)
add_fhmap_test(test_fhmap_entry)
add_fhmap_test(test_fhmap_iter)

#############  Doubly Linked List ##########################

add_library(dll_util dll/dll_util.h dll/dll_util.c)
target_link_libraries(dll_util
  ccc
)
add_dependencies(tests dll_util)

macro(add_dll_test TEST_NAME)
  add_executable(${TEST_NAME} dll/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers
      dll_util
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_dll_test(test_dll_construct)
add_dll_test(test_dll_insert)
add_dll_test(test_dll_erase)

#############  Singly Linked List ##########################

add_library(sll_util sll/sll_util.h sll/sll_util.c)
target_link_libraries(sll_util
  ccc
)
add_dependencies(tests sll_util)

macro(add_sll_test TEST_NAME)
  add_executable(${TEST_NAME} sll/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers 
      sll_util
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_sll_test(test_sll_construct)
add_sll_test(test_sll_insert)
add_sll_test(test_sll_erase)

#############  Flat Double Ended Queue ##########################

add_library(fdeq_util fdeq/fdeq_util.h fdeq/fdeq_util.c)
target_link_libraries(fdeq_util
  ccc
)
add_dependencies(tests fdeq_util)

macro(add_fdeq_test TEST_NAME)
  add_executable(${TEST_NAME} fdeq/${TEST_NAME}.c)
  target_include_directories(${TEST_NAME} PRIVATE "../util")
  target_link_libraries(${TEST_NAME}
    PRIVATE
      ccc
      checkers 
      fdeq_util
      alloc
  )
  set_target_properties(${TEST_NAME} 
    PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
  )
  add_dependencies(tests ${TEST_NAME})
endmacro()

add_fdeq_test(test_fdeq_construct)
add_fdeq_test(test_fdeq_insert)
add_fdeq_test(test_fdeq_erase)

