add_executable(maze maze.c)
target_link_libraries(maze PRIVATE
  cli
  random
  str_view::str_view
  map
  double_ended_priority_queue
)

add_executable(graph graph.c)
target_link_libraries(graph PRIVATE
  cli
  random
  str_view::str_view
  flat_hash_map
  map
  priority_queue
  queue
  flat_priority_queue
)

add_executable(lru lru.c)
target_link_libraries(lru PRIVATE
  flat_hash_map
  doubly_linked_list
  fhash_util
)
