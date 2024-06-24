# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/kodi-build-wp/build/libdvdread/src/libdvdread"
  "C:/kodi-build-wp/build/libdvdread/src/libdvdread-build"
  "C:/kodi/project/BuildDependencies/win10-arm"
  "C:/kodi-build-wp/build/libdvdread/tmp"
  "C:/kodi-build-wp/build/libdvdread/src/libdvdread-stamp"
  "C:/kodi/project/BuildDependencies/downloads"
  "C:/kodi-build-wp/build/libdvdread/src/libdvdread-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/kodi-build-wp/build/libdvdread/src/libdvdread-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/kodi-build-wp/build/libdvdread/src/libdvdread-stamp${cfgdir}") # cfgdir has leading slash
endif()
