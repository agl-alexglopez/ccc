add_library(types INTERFACE types.h)

add_library(buf buf.h ${CMAKE_SOURCE_DIR}/src/buf.c)
target_link_libraries(buf
  INTERFACE
    impl_buf
)

add_library(pqueue pqueue.h ${CMAKE_SOURCE_DIR}/src/pqueue.c)
target_link_libraries(pqueue 
  INTERFACE
    impl_pqueue
)

add_library(flat_pqueue flat_pqueue.h ${CMAKE_SOURCE_DIR}/src/flat_pqueue.c)
target_link_libraries(flat_pqueue 
  INTERFACE
    impl_flat_pqueue
)

add_library(flat_hash flat_hash.h ${CMAKE_SOURCE_DIR}/src/flat_hash.c)
target_link_libraries(flat_hash PRIVATE
  INTERFACE
    impl_flat_hash
)

add_library(depqueue depqueue.h ${CMAKE_SOURCE_DIR}/src/tree.c)
target_link_libraries(depqueue PRIVATE
  INTERFACE
    impl_tree
)

add_library(set set.h ${CMAKE_SOURCE_DIR}/src/tree.c)
target_link_libraries(set PRIVATE
  INTERFACE
    impl_tree
)
