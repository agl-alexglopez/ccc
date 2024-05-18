add_library(pqueue pqueue.h ${CMAKE_SOURCE_DIR}/src/splay_tree.c)
target_link_libraries(pqueue PRIVATE
  tree
)

add_library(set set.h ${CMAKE_SOURCE_DIR}/src/splay_tree.c)
target_link_libraries(set PRIVATE
  tree
)
