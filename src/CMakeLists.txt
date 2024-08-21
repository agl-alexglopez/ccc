add_library(impl_buf INTERFACE impl_buf.h)
target_link_libraries(impl_buf INTERFACE types)

add_library(impl_tree INTERFACE impl_tree.h)
target_link_libraries(impl_tree INTERFACE types)

add_library(impl_flat_pqueue INTERFACE impl_flat_pqueue.h)
target_link_libraries(impl_flat_pqueue INTERFACE buf types)

add_library(impl_pqueue INTERFACE impl_pqueue.h)
target_link_libraries(impl_pqueue INTERFACE types)

add_library(impl_flat_hash INTERFACE impl_flat_hash.h)
target_link_libraries(impl_flat_hash INTERFACE types buf)
