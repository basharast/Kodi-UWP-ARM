# Install script for directory: C:/kodi-build-wp/build/libnfs/src/libnfs

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/kodi/project/BuildDependencies/win10-arm")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config.cmake"
         "C:/kodi-build-wp/build/libnfs/src/libnfs-build/CMakeFiles/Export/a12d8493da470e4f10019f17257ab057/libnfs-config.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config.cmake")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs" TYPE FILE FILES "C:/kodi-build-wp/build/libnfs/src/libnfs-build/CMakeFiles/Export/a12d8493da470e4f10019f17257ab057/libnfs-config.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
     "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config-debug.cmake")
    if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs" TYPE FILE FILES "C:/kodi-build-wp/build/libnfs/src/libnfs-build/CMakeFiles/Export/a12d8493da470e4f10019f17257ab057/libnfs-config-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
     "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config-minsizerel.cmake")
    if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs" TYPE FILE FILES "C:/kodi-build-wp/build/libnfs/src/libnfs-build/CMakeFiles/Export/a12d8493da470e4f10019f17257ab057/libnfs-config-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
     "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config-relwithdebinfo.cmake")
    if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs" TYPE FILE FILES "C:/kodi-build-wp/build/libnfs/src/libnfs-build/CMakeFiles/Export/a12d8493da470e4f10019f17257ab057/libnfs-config-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
     "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config-release.cmake")
    if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs" TYPE FILE FILES "C:/kodi-build-wp/build/libnfs/src/libnfs-build/CMakeFiles/Export/a12d8493da470e4f10019f17257ab057/libnfs-config-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/FindNFS.cmake;C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs/libnfs-config-version.cmake")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/lib/cmake/libnfs" TYPE FILE FILES
    "C:/kodi-build-wp/build/libnfs/src/libnfs/cmake/FindNFS.cmake"
    "C:/kodi-build-wp/build/libnfs/src/libnfs-build/libnfs-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/kodi/project/BuildDependencies/win10-arm/lib/pkgconfig/libnfs.pc")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/lib/pkgconfig" TYPE FILE FILES "C:/kodi-build-wp/build/libnfs/src/libnfs-build/libnfs.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/kodi/project/BuildDependencies/win10-arm/include/nfsc")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/include" TYPE DIRECTORY FILES "C:/kodi-build-wp/build/libnfs/src/libnfs/include/nfsc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/kodi/project/BuildDependencies/win10-arm/include/nfsc/libnfs-raw-mount.h;C:/kodi/project/BuildDependencies/win10-arm/include/nfsc/libnfs-raw-nfs.h;C:/kodi/project/BuildDependencies/win10-arm/include/nfsc/libnfs-raw-nfs4.h;C:/kodi/project/BuildDependencies/win10-arm/include/nfsc/libnfs-raw-nlm.h;C:/kodi/project/BuildDependencies/win10-arm/include/nfsc/libnfs-raw-nsm.h;C:/kodi/project/BuildDependencies/win10-arm/include/nfsc/libnfs-raw-portmap.h;C:/kodi/project/BuildDependencies/win10-arm/include/nfsc/libnfs-raw-rquota.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/kodi/project/BuildDependencies/win10-arm/include/nfsc" TYPE FILE FILES
    "C:/kodi-build-wp/build/libnfs/src/libnfs/mount/libnfs-raw-mount.h"
    "C:/kodi-build-wp/build/libnfs/src/libnfs/nfs/libnfs-raw-nfs.h"
    "C:/kodi-build-wp/build/libnfs/src/libnfs/nfs4/libnfs-raw-nfs4.h"
    "C:/kodi-build-wp/build/libnfs/src/libnfs/nlm/libnfs-raw-nlm.h"
    "C:/kodi-build-wp/build/libnfs/src/libnfs/nsm/libnfs-raw-nsm.h"
    "C:/kodi-build-wp/build/libnfs/src/libnfs/portmap/libnfs-raw-portmap.h"
    "C:/kodi-build-wp/build/libnfs/src/libnfs/rquota/libnfs-raw-rquota.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/win32/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/mount/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/nfs/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/nfs4/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/nlm/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/nsm/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/portmap/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/rquota/cmake_install.cmake")
  include("C:/kodi-build-wp/build/libnfs/src/libnfs-build/lib/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/kodi-build-wp/build/libnfs/src/libnfs-build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
