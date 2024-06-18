add_executable(maze maze.c)
target_link_libraries(maze PRIVATE
  cli
  str_view
  set
  pqueue
)

add_executable(graph graph.c)
target_link_libraries(graph PRIVATE
  cli
  str_view
  set
  pqueue
  queue
  heap_pqueue
)

