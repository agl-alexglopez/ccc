add_library(buf buf.h ${CMAKE_SOURCE_DIR}/ccc_src/buf.c)
target_link_libraries(buf PRIVATE
  attrib
)

add_library(pqueue pqueue.h ${CMAKE_SOURCE_DIR}/ccc_src/pqueue.c)

add_library(flat_pqueue flat_pqueue.h ${CMAKE_SOURCE_DIR}/ccc_src/flat_pqueue.c)
target_link_libraries(flat_pqueue PRIVATE
  attrib
  buf
)

add_library(depqueue depqueue.h ${CMAKE_SOURCE_DIR}/ccc_src/splay_tree.c)
target_link_libraries(depqueue PRIVATE
  tree
  attrib
)

add_library(set set.h ${CMAKE_SOURCE_DIR}/ccc_src/splay_tree.c)
target_link_libraries(set PRIVATE
  tree
  attrib
)
