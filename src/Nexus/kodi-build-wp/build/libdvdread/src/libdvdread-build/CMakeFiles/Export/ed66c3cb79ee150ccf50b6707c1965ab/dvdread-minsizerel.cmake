#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dvdread::dvdread" for configuration "MinSizeRel"
set_property(TARGET dvdread::dvdread APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(dvdread::dvdread PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/dvdread.lib"
  )

list(APPEND _cmake_import_check_targets dvdread::dvdread )
list(APPEND _cmake_import_check_files_for_dvdread::dvdread "${_IMPORT_PREFIX}/lib/dvdread.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
