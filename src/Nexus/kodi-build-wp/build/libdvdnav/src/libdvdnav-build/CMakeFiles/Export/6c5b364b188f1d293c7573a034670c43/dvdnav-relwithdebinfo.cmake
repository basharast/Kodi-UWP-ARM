#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dvdnav::dvdnav" for configuration "RelWithDebInfo"
set_property(TARGET dvdnav::dvdnav APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(dvdnav::dvdnav PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libdvdnav.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/libdvdnav.dll"
  )

list(APPEND _cmake_import_check_targets dvdnav::dvdnav )
list(APPEND _cmake_import_check_files_for_dvdnav::dvdnav "${_IMPORT_PREFIX}/lib/libdvdnav.lib" "${_IMPORT_PREFIX}/bin/libdvdnav.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
