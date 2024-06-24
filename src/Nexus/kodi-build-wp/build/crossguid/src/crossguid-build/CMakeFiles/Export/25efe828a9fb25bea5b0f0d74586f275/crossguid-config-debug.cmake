#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "crossguid" for configuration "Debug"
set_property(TARGET crossguid APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(crossguid PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/crossguid-dgb.lib"
  )

list(APPEND _cmake_import_check_targets crossguid )
list(APPEND _cmake_import_check_files_for_crossguid "${_IMPORT_PREFIX}/lib/crossguid-dgb.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
