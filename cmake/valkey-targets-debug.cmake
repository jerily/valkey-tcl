#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valkey::valkey" for configuration "Debug"
set_property(TARGET valkey::valkey APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(valkey::valkey PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libvalkey.so.1.2.1-dev"
  IMPORTED_SONAME_DEBUG "libvalkey.so.1.2.1-dev"
  )

list(APPEND _cmake_import_check_targets valkey::valkey )
list(APPEND _cmake_import_check_files_for_valkey::valkey "${_IMPORT_PREFIX}/lib/libvalkey.so.1.2.1-dev" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
