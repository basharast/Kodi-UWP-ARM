# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix/src/dvdread"
  "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix/src/dvdread-build"
  "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix"
  "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix/tmp"
  "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix/src/dvdread-stamp"
  "C:/kodi/project/BuildDependencies/downloads"
  "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix/src/dvdread-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix/src/dvdread-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/kodi/tools/buildsteps/windows/win32-uwp/dvdread-prefix/src/dvdread-stamp${cfgdir}") # cfgdir has leading slash
endif()
