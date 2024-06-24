# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/kodi-build-wp/build/flatc/src/flatc"
  "C:/kodi-build-wp/build/flatc/src/flatc-build"
  "C:/kodi/project/BuildDependencies/tools/bin"
  "C:/kodi-build-wp/build/flatc/tmp"
  "C:/kodi-build-wp/build/flatc/src/flatc-stamp"
  "C:/kodi/project/BuildDependencies/downloads"
  "C:/kodi-build-wp/build/flatc/src/flatc-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/kodi-build-wp/build/flatc/src/flatc-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/kodi-build-wp/build/flatc/src/flatc-stamp${cfgdir}") # cfgdir has leading slash
endif()
