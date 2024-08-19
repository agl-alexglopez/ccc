add_library(tree INTERFACE tree.h)
target_link_libraries(tree INTERFACE attrib)

add_library(impl_flat_pqueue impl_flat_pqueue.h flat_pqueue.c)
target_link_libraries(impl_flat_pqueue PRIVATE buf)
