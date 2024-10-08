add_library(impl_types INTERFACE impl_types.h)

add_library(impl_buffer INTERFACE impl_buffer.h)
target_link_libraries(impl_buffer INTERFACE types)

add_library(impl_doubly_linked_list INTERFACE impl_doubly_linked_list.h)
target_link_libraries(impl_doubly_linked_list INTERFACE types)

add_library(impl_singly_linked_list INTERFACE impl_singly_linked_list.h)
target_link_libraries(impl_singly_linked_list INTERFACE types)

add_library(impl_tree INTERFACE impl_tree.h)
target_link_libraries(impl_tree INTERFACE types)

add_library(impl_ordered_map INTERFACE impl_ordered_map.h)
target_link_libraries(impl_ordered_map INTERFACE impl_tree)

add_library(impl_flat_ordered_map INTERFACE impl_flat_ordered_map.h)
target_link_libraries(impl_flat_ordered_map INTERFACE buffer types)

add_library(impl_double_ended_priority_queue INTERFACE impl_double_ended_priority_queue.h)
target_link_libraries(impl_double_ended_priority_queue INTERFACE impl_tree)

add_library(impl_realtime_ordered_map INTERFACE impl_realtime_ordered_map.h)
target_link_libraries(impl_realtime_ordered_map INTERFACE types)

add_library(impl_flat_realtime_ordered_map INTERFACE impl_flat_realtime_ordered_map.h)
target_link_libraries(impl_flat_realtime_ordered_map INTERFACE buffer types)

add_library(impl_flat_double_ended_queue INTERFACE impl_flat_double_ended_queue.h)
target_link_libraries(impl_flat_double_ended_queue INTERFACE buffer types)

add_library(impl_flat_priority_queue INTERFACE impl_flat_priority_queue.h)
target_link_libraries(impl_flat_priority_queue INTERFACE buffer types)

add_library(impl_priority_queue INTERFACE impl_priority_queue.h)
target_link_libraries(impl_priority_queue INTERFACE types)

add_library(impl_flat_hash_map INTERFACE impl_flat_hash_map.h)
target_link_libraries(impl_flat_hash_map INTERFACE types buffer)

add_library(impl_traits INTERFACE impl_traits.h)
target_link_libraries(impl_traits 
  INTERFACE
    types
    buffer
    doubly_linked_list
    singly_linked_list
    ordered_map
    flat_ordered_map
    realtime_ordered_map
    flat_realtime_ordered_map
    double_ended_priority_queue
    flat_hash_map
    flat_double_ended_queue
    flat_priority_queue
    priority_queue
)
