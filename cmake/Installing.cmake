
# for CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_INCLUDEDIR and others
include(GNUInstallDirs)

# Keeping the project name within a cmake folder helps reduce file clutter
set(INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" 
    CACHE STRING "Path to ${PROJECT_NAME} CMake files"
)

# Default destinations from GNUInstallDirs are only changed for debug config.
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    install(TARGETS ${PROJECT_NAME}
        EXPORT "${PROJECT_NAME}Targets"
        FILE_SET public_headers
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/debug/bin # bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/debug # lib/debug
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/debug # lib/debug
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME} # include/ccc
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include
    )
else()
    install(TARGETS ${PROJECT_NAME}
        EXPORT "${PROJECT_NAME}Targets"
        FILE_SET public_headers
        # All destinations are default in release except header location.
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME} # include/str_view/
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include
    )
endif()

# generate and install export file
install(EXPORT "${PROJECT_NAME}Targets"
    FILE "${PROJECT_NAME}Targets.cmake"
    NAMESPACE ${namespace}::
    DESTINATION "${INSTALL_CMAKEDIR}"
)

include(CMakePackageConfigHelpers)

# generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    COMPATIBILITY AnyNewerVersion
)
# create config file
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${INSTALL_CMAKEDIR}"
)
# install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${INSTALL_CMAKEDIR}"
)
