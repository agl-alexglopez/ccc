add_executable(maze maze.c)
target_link_libraries(maze PRIVATE
  str_view
  set
  pqueue
)
