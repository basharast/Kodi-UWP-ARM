#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libnfs::nfs" for configuration "Release"
set_property(TARGET libnfs::nfs APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libnfs::nfs PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/nfs.lib"
  )

list(APPEND _cmake_import_check_targets libnfs::nfs )
list(APPEND _cmake_import_check_files_for_libnfs::nfs "${_IMPORT_PREFIX}/lib/nfs.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
