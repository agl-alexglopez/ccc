add_library(buf buf.h ${CMAKE_SOURCE_DIR}/ccc_src/buf.c)

add_library(pqueue pqueue.h ${CMAKE_SOURCE_DIR}/ccc_src/pqueue.c)

add_library(flat_pqueue flat_pqueue.h ${CMAKE_SOURCE_DIR}/ccc_src/flat_pqueue.c)
target_link_libraries(flat_pqueue PRIVATE
  buf
)

add_library(depqueue depqueue.h ${CMAKE_SOURCE_DIR}/ccc_src/splay_tree.c)
target_link_libraries(depqueue PRIVATE
  tree
)

add_library(set set.h ${CMAKE_SOURCE_DIR}/ccc_src/splay_tree.c)
target_link_libraries(set PRIVATE
  tree
)
