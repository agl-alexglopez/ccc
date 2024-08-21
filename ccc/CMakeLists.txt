add_library(types INTERFACE types.h)
add_library(buf buf.h ${CMAKE_SOURCE_DIR}/src/buf.c)

add_library(pqueue pqueue.h ${CMAKE_SOURCE_DIR}/src/pqueue.c)
target_link_libraries(pqueue 
  PRIVATE
    types
)

add_library(flat_pqueue flat_pqueue.h ${CMAKE_SOURCE_DIR}/src/flat_pqueue.c)
target_link_libraries(flat_pqueue 
  PRIVATE
    types
    impl_flat_pqueue
)

add_library(flat_hash flat_hash.h ${CMAKE_SOURCE_DIR}/src/flat_hash.c)
target_link_libraries(flat_hash PRIVATE
  PRIVATE
    types
    buf
)

add_library(depqueue depqueue.h ${CMAKE_SOURCE_DIR}/src/tree.c)
target_link_libraries(depqueue PRIVATE
  PRIVATE
    types
    impl_tree
)

add_library(set set.h ${CMAKE_SOURCE_DIR}/src/tree.c)
target_link_libraries(set PRIVATE
  PRIVATE
    types
    impl_tree
)
