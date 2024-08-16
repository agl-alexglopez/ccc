add_library(buf buf.h ${CMAKE_SOURCE_DIR}/src/buf.c)

add_library(pqueue pqueue.h ${CMAKE_SOURCE_DIR}/src/pqueue.c)

add_library(flat_pqueue flat_pqueue.h ${CMAKE_SOURCE_DIR}/src/flat_pqueue.c)
target_link_libraries(flat_pqueue PRIVATE
  buf
)

add_library(flat_hash flat_hash.h ${CMAKE_SOURCE_DIR}/src/flat_hash.c)
target_link_libraries(flat_hash PRIVATE
  buf
)

add_library(depqueue depqueue.h ${CMAKE_SOURCE_DIR}/src/splay_tree.c)
target_link_libraries(depqueue PRIVATE
  tree
)

add_library(set set.h ${CMAKE_SOURCE_DIR}/src/splay_tree.c)
target_link_libraries(set PRIVATE
  tree
)
