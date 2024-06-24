#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dvdcss::dvdcss" for configuration "Debug"
set_property(TARGET dvdcss::dvdcss APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dvdcss::dvdcss PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/dvdcss.lib"
  )

list(APPEND _cmake_import_check_targets dvdcss::dvdcss )
list(APPEND _cmake_import_check_files_for_dvdcss::dvdcss "${_IMPORT_PREFIX}/lib/dvdcss.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
