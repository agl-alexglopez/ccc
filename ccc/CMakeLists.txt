add_library(types INTERFACE types.h)

add_library(generics generics.h ${CMAKE_SOURCE_DIR}/src/generics.c)
target_link_libraries(generics
  PRIVATE
    impl_generics
)

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

add_library(realtime_ordered_map realtime_ordered_map.h ${CMAKE_SOURCE_DIR}/src/realtime_ordered_map.c)
target_link_libraries(realtime_ordered_map
  INTERFACE
    types
  PRIVATE
    impl_realtime_ordered_map
)

add_library(flat_queue flat_queue.h ${CMAKE_SOURCE_DIR}/src/flat_queue.c)
target_link_libraries(flat_queue
  INTERFACE
    types
  PRIVATE
    impl_flat_queue
)

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# add sources to the library target
target_sources(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/generics.c
        ${PROJECT_SOURCE_DIR}/src/impl_buffer.h
        ${PROJECT_SOURCE_DIR}/src/buffer.c
        ${PROJECT_SOURCE_DIR}/src/impl_flat_hash_map.h
        ${PROJECT_SOURCE_DIR}/src/flat_hash_map.c
        ${PROJECT_SOURCE_DIR}/src/impl_flat_queue.h
        ${PROJECT_SOURCE_DIR}/src/flat_queue.c
        ${PROJECT_SOURCE_DIR}/src/impl_flat_priority_queue.h
        ${PROJECT_SOURCE_DIR}/src/flat_priority_queue.c
        ${PROJECT_SOURCE_DIR}/src/impl_tree.h
        ${PROJECT_SOURCE_DIR}/src/tree.c
        ${PROJECT_SOURCE_DIR}/src/impl_priority_queue.h
        ${PROJECT_SOURCE_DIR}/src/priority_queue.c
        ${PROJECT_SOURCE_DIR}/src/impl_singly_linked_list.h
        ${PROJECT_SOURCE_DIR}/src/singly_linked_list.c
        ${PROJECT_SOURCE_DIR}/src/impl_doubly_linked_list.h
        ${PROJECT_SOURCE_DIR}/src/doubly_linked_list.c
        ${PROJECT_SOURCE_DIR}/src/impl_realtime_ordered_map.h
        ${PROJECT_SOURCE_DIR}/src/realtime_ordered_map.c
    PUBLIC 
        FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              types.h
              generics.h
              buffer.h
              flat_hash_map.h
              flat_queue.h
              flat_priority_queue.h
              ordered_map.h
              realtime_ordered_map.h
              double_ended_priority_queue.h
              priority_queue.h
              singly_linked_list.h
              doubly_linked_list.h
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
