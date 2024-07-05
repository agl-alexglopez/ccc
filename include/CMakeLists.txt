add_library(pqueue pqueue.h ${CMAKE_SOURCE_DIR}/src/pqueue.c)

add_library(depqueue depqueue.h ${CMAKE_SOURCE_DIR}/src/splay_tree.c)
target_link_libraries(depqueue PRIVATE
  tree
  attrib
)

add_library(set set.h ${CMAKE_SOURCE_DIR}/src/splay_tree.c)
target_link_libraries(set PRIVATE
  tree
  attrib
)
