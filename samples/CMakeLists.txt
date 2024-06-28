add_executable(maze maze.c)
target_link_libraries(maze PRIVATE
  cli
  random
  str_view
  set
  depqueue
)

add_executable(graph graph.c)
target_link_libraries(graph PRIVATE
  cli
  random
  str_view
  set
  pair_pqueue
  queue
  heap_pqueue
)

