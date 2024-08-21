add_library(impl_tree INTERFACE impl_tree.h)

add_library(impl_flat_pqueue INTERFACE impl_flat_pqueue.h)
target_link_libraries(impl_flat_pqueue INTERFACE buf)

add_library(impl_pqueue INTERFACE impl_pqueue.h)
