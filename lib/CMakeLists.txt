add_library(tree INTERFACE tree.h)

add_library(pqueue pqueue.h splay_tree.c)
target_link_libraries(pqueue
  tree
)

add_library(set set.h splay_tree.c)
target_link_libraries(set
  tree
)
