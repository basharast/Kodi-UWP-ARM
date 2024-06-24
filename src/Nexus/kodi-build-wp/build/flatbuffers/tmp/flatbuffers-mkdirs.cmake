# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/kodi-build-wp/build/flatbuffers/src/flatbuffers"
  "C:/kodi-build-wp/build/flatbuffers/src/flatbuffers-build"
  "C:/kodi/project/BuildDependencies/win10-arm"
  "C:/kodi-build-wp/build/flatbuffers/tmp"
  "C:/kodi-build-wp/build/flatbuffers/src/flatbuffers-stamp"
  "C:/kodi/project/BuildDependencies/downloads"
  "C:/kodi-build-wp/build/flatbuffers/src/flatbuffers-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/kodi-build-wp/build/flatbuffers/src/flatbuffers-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/kodi-build-wp/build/flatbuffers/src/flatbuffers-stamp${cfgdir}") # cfgdir has leading slash
endif()
