#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dvdcss::dvdcss" for configuration "Release"
set_property(TARGET dvdcss::dvdcss APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dvdcss::dvdcss PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/dvdcss.lib"
  )

list(APPEND _cmake_import_check_targets dvdcss::dvdcss )
list(APPEND _cmake_import_check_files_for_dvdcss::dvdcss "${_IMPORT_PREFIX}/lib/dvdcss.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
