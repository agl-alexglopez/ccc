add_library(types)
target_sources(types
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/types.c
        impl_types.h
    PUBLIC 
        FILE_SET types_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              types.h
)

add_library(buffer)
target_sources(buffer
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/buffer.c
        impl_buffer.h
    PUBLIC 
        FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              buffer.h
)
target_link_libraries(buffer
  PUBLIC
    types
)

add_library(doubly_linked_list)
target_sources(doubly_linked_list
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/doubly_linked_list.c
        impl_doubly_linked_list.h
    PUBLIC 
        FILE_SET doubly_linked_list_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              doubly_linked_list.h
)
target_link_libraries(doubly_linked_list
  PUBLIC
    types
)

add_library(singly_linked_list)
target_sources(singly_linked_list
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/singly_linked_list.c
        impl_singly_linked_list.h
    PUBLIC 
        FILE_SET singly_linked_list_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              singly_linked_list.h
)
target_link_libraries(singly_linked_list
  PUBLIC
    types
)

add_library(priority_queue)
target_sources(priority_queue
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/priority_queue.c
        impl_priority_queue.h
    PUBLIC 
        FILE_SET priority_queue_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              priority_queue.h
)
target_link_libraries(priority_queue
  PUBLIC
    types
)

add_library(flat_priority_queue)
target_sources(flat_priority_queue
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/flat_priority_queue.c
        impl_flat_priority_queue.h
    PUBLIC 
        FILE_SET buffer_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              flat_priority_queue.h
)
target_link_libraries(flat_priority_queue 
  PUBLIC
    types
  PRIVATE
    buffer
)

add_library(flat_hash_map)
target_sources(flat_hash_map
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/flat_hash_map.c
        impl_flat_hash_map.h
    PUBLIC 
        FILE_SET flat_hash_map_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              flat_hash_map.h
)
target_link_libraries(flat_hash_map 
  PUBLIC
    types
  PRIVATE
    buffer
)

add_library(ordered_multimap)
target_sources(ordered_multimap
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ordered_multimap.c
        impl_ordered_multimap.h
    PUBLIC 
        FILE_SET ordered_multimap_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              ordered_multimap.h
)
target_link_libraries(ordered_multimap
  PUBLIC
    types
)

add_library(ordered_map)
target_sources(ordered_map
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ordered_map.c
        impl_ordered_map.h
    PUBLIC 
        FILE_SET ordered_map_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              ordered_map.h
)
target_link_libraries(ordered_map
  PUBLIC
    types
)

add_library(flat_ordered_map)
target_sources(flat_ordered_map
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/flat_ordered_map.c
        impl_flat_ordered_map.h
    PUBLIC 
        FILE_SET flat_ordered_map_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              flat_ordered_map.h
)
target_link_libraries(flat_ordered_map
  PUBLIC
    types
  PRIVATE
    buffer
)

add_library(realtime_ordered_map)
target_sources(realtime_ordered_map
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/realtime_ordered_map.c
        impl_realtime_ordered_map.h
    PUBLIC 
        FILE_SET realtime_ordered_map_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              realtime_ordered_map.h
)
target_link_libraries(realtime_ordered_map
  PUBLIC
    types
)

add_library(flat_realtime_ordered_map)
target_sources(flat_realtime_ordered_map
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/flat_realtime_ordered_map.c
        impl_flat_realtime_ordered_map.h
    PUBLIC 
        FILE_SET flat_realtime_ordered_map_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              flat_realtime_ordered_map.h
)
target_link_libraries(flat_realtime_ordered_map
  PUBLIC
    types
  PRIVATE
    buffer
)

add_library(flat_double_ended_queue)
target_sources(flat_double_ended_queue
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/flat_double_ended_queue.c
        impl_flat_double_ended_queue.h
    PUBLIC 
        FILE_SET flat_double_ended_queue_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              flat_double_ended_queue.h
)
target_link_libraries(flat_double_ended_queue
  PUBLIC
    types
  PRIVATE
    buffer
)

add_library(traits INTERFACE)
target_sources(traits
    PRIVATE
        impl_traits.h
    INTERFACE
        FILE_SET traits_header
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              traits.h
)
target_link_libraries(traits
    INTERFACE
        types
        buffer
        doubly_linked_list
        singly_linked_list
        priority_queue
        flat_priority_queue
        ordered_multimap
        ordered_map
        realtime_ordered_map
        flat_realtime_ordered_map
        flat_hash_map
        flat_double_ended_queue
)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# add sources to the library target
target_sources(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/types.c
        ${PROJECT_SOURCE_DIR}/src/buffer.c
        ${PROJECT_SOURCE_DIR}/src/flat_hash_map.c
        ${PROJECT_SOURCE_DIR}/src/flat_double_ended_queue.c
        ${PROJECT_SOURCE_DIR}/src/flat_priority_queue.c
        ${PROJECT_SOURCE_DIR}/src/ordered_map.c
        ${PROJECT_SOURCE_DIR}/src/flat_ordered_map.c
        ${PROJECT_SOURCE_DIR}/src/ordered_multimap.c
        ${PROJECT_SOURCE_DIR}/src/priority_queue.c
        ${PROJECT_SOURCE_DIR}/src/singly_linked_list.c
        ${PROJECT_SOURCE_DIR}/src/doubly_linked_list.c
        ${PROJECT_SOURCE_DIR}/src/realtime_ordered_map.c
        ${PROJECT_SOURCE_DIR}/src/flat_realtime_ordered_map.c
    PUBLIC 
        FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              impl_types.h
              impl_flat_priority_queue.h
              impl_tree.h
              impl_priority_queue.h
              impl_ordered_map.h
              impl_flat_ordered_map.h
              impl_ordered_multimap.h
              impl_singly_linked_list.h
              impl_doubly_linked_list.h
              impl_realtime_ordered_map.h
              impl_flat_realtime_ordered_map.h
              impl_traits.h
              impl_flat_double_ended_queue.h
              impl_flat_hash_map.h
              impl_buffer.h
              types.h
              buffer.h
              flat_hash_map.h
              flat_double_ended_queue.h
              flat_priority_queue.h
              ordered_map.h
              flat_ordered_map.h
              realtime_ordered_map.h
              flat_realtime_ordered_map.h
              ordered_multimap.h
              priority_queue.h
              singly_linked_list.h
              doubly_linked_list.h
              traits.h
)

target_compile_features(${PROJECT_NAME} PUBLIC c_std_23)

# set properties for the target. VERSION set the library version to the project
# version * SOVERSION set the compatibility  version for the library to the
# major number of the version
# note that ${public_headers} should be in quotes
set_target_properties(${PROJECT_NAME} 
    PROPERTIES 
        RELEASE_POSTFIX "_release"
        DEBUG_POSTFIX "_debug"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        C_VISIBILITY_PRESET "default"
        VISIBILITY_INLINES_HIDDEN TRUE
)

include(GenerateExportHeader)

# this command generates a header file in the CMAKE_CURRENT_BINARY_DIR which
# sets the visibility attributes according to the compiler settings
generate_export_header(${PROJECT_NAME}
    EXPORT_FILE_NAME export/export_${PROJECT_NAME}.h
)

# Add CMAKE_CURRENT_BINARY_DIR to the include path so the generated header can
# be found
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
