# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/kodi-build-wp/build/taglib/src/taglib"
  "C:/kodi-build-wp/build/taglib/src/taglib-build"
  "C:/kodi/project/BuildDependencies/win10-arm"
  "C:/kodi-build-wp/build/taglib/tmp"
  "C:/kodi-build-wp/build/taglib/src/taglib-stamp"
  "C:/kodi/project/BuildDependencies/downloads"
  "C:/kodi-build-wp/build/taglib/src/taglib-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/kodi-build-wp/build/taglib/src/taglib-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/kodi-build-wp/build/taglib/src/taglib-stamp${cfgdir}") # cfgdir has leading slash
endif()
