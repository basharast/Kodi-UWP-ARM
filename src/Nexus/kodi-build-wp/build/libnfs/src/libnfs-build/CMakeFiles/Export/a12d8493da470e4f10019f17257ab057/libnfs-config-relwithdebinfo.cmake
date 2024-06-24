#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libnfs::nfs" for configuration "RelWithDebInfo"
set_property(TARGET libnfs::nfs APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(libnfs::nfs PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/nfs.lib"
  )

list(APPEND _cmake_import_check_targets libnfs::nfs )
list(APPEND _cmake_import_check_files_for_libnfs::nfs "${_IMPORT_PREFIX}/lib/nfs.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
