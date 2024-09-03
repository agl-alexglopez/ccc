#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ccc::ccc" for configuration "Release"
set_property(TARGET ccc::ccc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ccc::ccc PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libccc_release.a"
  )

list(APPEND _cmake_import_check_targets ccc::ccc )
list(APPEND _cmake_import_check_files_for_ccc::ccc "${_IMPORT_PREFIX}/lib/libccc_release.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
