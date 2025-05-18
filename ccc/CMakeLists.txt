add_library(${PROJECT_NAME})
add_library(${namespace}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# File sets simplify install and allow isolation of only container files.
target_sources(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/types.c
        ${PROJECT_SOURCE_DIR}/src/buffer.c
        ${PROJECT_SOURCE_DIR}/src/flat_hash_map.c
        ${PROJECT_SOURCE_DIR}/src/flat_double_ended_queue.c
        ${PROJECT_SOURCE_DIR}/src/flat_priority_queue.c
        ${PROJECT_SOURCE_DIR}/src/ordered_map.c
        ${PROJECT_SOURCE_DIR}/src/handle_ordered_map.c
        ${PROJECT_SOURCE_DIR}/src/ordered_multimap.c
        ${PROJECT_SOURCE_DIR}/src/priority_queue.c
        ${PROJECT_SOURCE_DIR}/src/singly_linked_list.c
        ${PROJECT_SOURCE_DIR}/src/doubly_linked_list.c
        ${PROJECT_SOURCE_DIR}/src/realtime_ordered_map.c
        ${PROJECT_SOURCE_DIR}/src/handle_realtime_ordered_map.c
        ${PROJECT_SOURCE_DIR}/src/bitset.c
    PUBLIC 
        FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}
            FILES 
              impl/impl_types.h
              impl/impl_flat_priority_queue.h
              impl/impl_priority_queue.h
              impl/impl_ordered_map.h
              impl/impl_handle_ordered_map.h
              impl/impl_ordered_multimap.h
              impl/impl_singly_linked_list.h
              impl/impl_doubly_linked_list.h
              impl/impl_realtime_ordered_map.h
              impl/impl_handle_realtime_ordered_map.h
              impl/impl_traits.h
              impl/impl_flat_double_ended_queue.h
              impl/impl_flat_hash_map.h
              impl/impl_buffer.h
              impl/impl_bitset.h
              types.h
              buffer.h
              bitset.h
              flat_hash_map.h
              flat_double_ended_queue.h
              flat_priority_queue.h
              ordered_map.h
              handle_ordered_map.h
              realtime_ordered_map.h
              handle_realtime_ordered_map.h
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
