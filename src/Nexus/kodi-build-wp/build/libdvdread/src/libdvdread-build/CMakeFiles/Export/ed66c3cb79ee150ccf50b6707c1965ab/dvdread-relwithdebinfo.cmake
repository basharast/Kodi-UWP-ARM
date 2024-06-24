#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dvdread::dvdread" for configuration "RelWithDebInfo"
set_property(TARGET dvdread::dvdread APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(dvdread::dvdread PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/dvdread.lib"
  )

list(APPEND _cmake_import_check_targets dvdread::dvdread )
list(APPEND _cmake_import_check_files_for_dvdread::dvdread "${_IMPORT_PREFIX}/lib/dvdread.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
