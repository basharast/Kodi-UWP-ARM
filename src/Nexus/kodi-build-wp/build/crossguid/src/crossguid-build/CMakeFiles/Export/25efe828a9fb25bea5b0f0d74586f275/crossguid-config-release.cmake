#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "crossguid" for configuration "Release"
set_property(TARGET crossguid APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(crossguid PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/crossguid.lib"
  )

list(APPEND _cmake_import_check_targets crossguid )
list(APPEND _cmake_import_check_files_for_crossguid "${_IMPORT_PREFIX}/lib/crossguid.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
