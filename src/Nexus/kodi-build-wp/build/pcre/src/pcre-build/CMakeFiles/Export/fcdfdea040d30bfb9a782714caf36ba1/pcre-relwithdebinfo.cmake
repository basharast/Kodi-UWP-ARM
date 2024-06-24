#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "PCRE::pcre" for configuration "RelWithDebInfo"
set_property(TARGET PCRE::pcre APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(PCRE::pcre PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/pcre.lib"
  )

list(APPEND _cmake_import_check_targets PCRE::pcre )
list(APPEND _cmake_import_check_files_for_PCRE::pcre "${_IMPORT_PREFIX}/lib/pcre.lib" )

# Import target "PCRE::pcreposix" for configuration "RelWithDebInfo"
set_property(TARGET PCRE::pcreposix APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(PCRE::pcreposix PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "PCRE::pcre"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/pcreposix.lib"
  )

list(APPEND _cmake_import_check_targets PCRE::pcreposix )
list(APPEND _cmake_import_check_files_for_PCRE::pcreposix "${_IMPORT_PREFIX}/lib/pcreposix.lib" )

# Import target "PCRE::pcrecpp" for configuration "RelWithDebInfo"
set_property(TARGET PCRE::pcrecpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(PCRE::pcrecpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "PCRE::pcre"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/pcrecpp.lib"
  )

list(APPEND _cmake_import_check_targets PCRE::pcrecpp )
list(APPEND _cmake_import_check_files_for_PCRE::pcrecpp "${_IMPORT_PREFIX}/lib/pcrecpp.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
