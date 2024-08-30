add_executable(maze maze.c)
target_link_libraries(maze PRIVATE
  cli
  random
  str_view::str_view
  set
  double_ended_priority_queue
)

add_executable(graph graph.c)
target_link_libraries(graph PRIVATE
  cli
  random
  str_view::str_view
  flat_hash
  set
  priority_queue
  queue
  flat_priority_queue
)

