add_library(types INTERFACE types.h)

add_library(buffer buffer.h ${CMAKE_SOURCE_DIR}/src/buffer.c)
target_link_libraries(buffer
  INTERFACE
    types
  PRIVATE
    impl_buffer
)

add_library(doubly_linked_list doubly_linked_list.h ${CMAKE_SOURCE_DIR}/src/doubly_linked_list.c)
target_link_libraries(doubly_linked_list
  INTERFACE
    types
  PRIVATE
    impl_doubly_linked_list
)

add_library(singly_linked_list singly_linked_list.h ${CMAKE_SOURCE_DIR}/src/singly_linked_list.c)
target_link_libraries(singly_linked_list
  INTERFACE
    types
  PRIVATE
    impl_singly_linked_list
)

add_library(priority_queue priority_queue.h ${CMAKE_SOURCE_DIR}/src/priority_queue.c)
target_link_libraries(priority_queue 
  INTERFACE
    types
  PRIVATE
    impl_priority_queue
)

add_library(flat_priority_queue flat_priority_queue.h ${CMAKE_SOURCE_DIR}/src/flat_priority_queue.c)
target_link_libraries(flat_priority_queue 
  INTERFACE
    types
  PRIVATE
    impl_flat_priority_queue
)

add_library(flat_hash_map flat_hash_map.h ${CMAKE_SOURCE_DIR}/src/flat_hash_map.c)
target_link_libraries(flat_hash_map
  INTERFACE
    types
  PRIVATE
    impl_flat_hash_map
)

add_library(double_ended_priority_queue double_ended_priority_queue.h ${CMAKE_SOURCE_DIR}/src/tree.c)
target_link_libraries(double_ended_priority_queue
  INTERFACE
    types
  PRIVATE
    impl_tree
)

add_library(ordered_map ordered_map.h ${CMAKE_SOURCE_DIR}/src/tree.c)
target_link_libraries(ordered_map
  INTERFACE
    types
  PRIVATE
    impl_tree
)
