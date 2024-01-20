add_executable(pq_tests 
  pq_tests.c
)
target_link_libraries(pq_tests 
  pqueue
)

add_executable(set_tests 
  set_tests.c
)
target_link_libraries(set_tests
  set
)

