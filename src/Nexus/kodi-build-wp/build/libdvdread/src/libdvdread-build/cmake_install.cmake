# Install script for directory: C:/kodi-build-wp/build/libdvdread/src/libdvdread

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
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/Debug/dvdread.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/Release/dvdread.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/MinSizeRel/dvdread.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/RelWithDebInfo/dvdread.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dvdread" TYPE FILE FILES
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/bitreader.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/dvd_reader.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/dvd_udf.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/ifo_print.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/ifo_read.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/ifo_types.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/nav_print.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/nav_read.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/src/dvdread/nav_types.h"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/version.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread/dvdread.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread/dvdread.cmake"
         "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/CMakeFiles/Export/ed66c3cb79ee150ccf50b6707c1965ab/dvdread.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread/dvdread-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread/dvdread.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread" TYPE FILE FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/CMakeFiles/Export/ed66c3cb79ee150ccf50b6707c1965ab/dvdread.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread" TYPE FILE FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/CMakeFiles/Export/ed66c3cb79ee150ccf50b6707c1965ab/dvdread-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread" TYPE FILE FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/CMakeFiles/Export/ed66c3cb79ee150ccf50b6707c1965ab/dvdread-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread" TYPE FILE FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/CMakeFiles/Export/ed66c3cb79ee150ccf50b6707c1965ab/dvdread-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread" TYPE FILE FILES "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/CMakeFiles/Export/ed66c3cb79ee150ccf50b6707c1965ab/dvdread-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/dvdread" TYPE FILE FILES
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread/cmake/dvdread-config.cmake"
    "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/dvdread-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
