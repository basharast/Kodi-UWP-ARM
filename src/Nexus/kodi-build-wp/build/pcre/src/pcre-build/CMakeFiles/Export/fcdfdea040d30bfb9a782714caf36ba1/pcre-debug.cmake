#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "PCRE::pcre" for configuration "Debug"
set_property(TARGET PCRE::pcre APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(PCRE::pcre PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/pcred.lib"
  )

list(APPEND _cmake_import_check_targets PCRE::pcre )
list(APPEND _cmake_import_check_files_for_PCRE::pcre "${_IMPORT_PREFIX}/lib/pcred.lib" )

# Import target "PCRE::pcreposix" for configuration "Debug"
set_property(TARGET PCRE::pcreposix APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(PCRE::pcreposix PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "PCRE::pcre"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/pcreposixd.lib"
  )

list(APPEND _cmake_import_check_targets PCRE::pcreposix )
list(APPEND _cmake_import_check_files_for_PCRE::pcreposix "${_IMPORT_PREFIX}/lib/pcreposixd.lib" )

# Import target "PCRE::pcrecpp" for configuration "Debug"
set_property(TARGET PCRE::pcrecpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(PCRE::pcrecpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "PCRE::pcre"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/pcrecppd.lib"
  )

list(APPEND _cmake_import_check_targets PCRE::pcrecpp )
list(APPEND _cmake_import_check_files_for_PCRE::pcrecpp "${_IMPORT_PREFIX}/lib/pcrecppd.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
