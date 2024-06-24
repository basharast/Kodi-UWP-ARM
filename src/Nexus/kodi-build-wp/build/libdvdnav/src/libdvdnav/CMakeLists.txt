cmake_minimum_required(VERSION 3.1)

set(DVDNAV_VERSION_MAJOR 5)
set(DVDNAV_VERSION_MINOR 0)
set(DVDNAV_VERSION_MICRO 4)
set(DVDNAV_VERSION "${DVDNAV_VERSION_MAJOR}.${DVDNAV_VERSION_MINOR}.${DVDNAV_VERSION_MICRO}")
set(PACKAGE_VERSION ${DVDNAV_VERSION})

project(dvdnav LANGUAGES C VERSION ${DVDNAV_VERSION})

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    ${CMAKE_MODULE_PATH})

#if(MSVC)
#  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/msvc/include
#                              ${CMAKE_REQUIRED_INCLUDES})
#endif()

include(CheckIncludeFile)
include(CheckFunctionExists)
include(TestLargeFiles)
include(CheckHeadersSTDC)
include(CheckFileOffsetBits)

find_package(dvdcss NO_MODULE)
find_package(dvdread REQUIRED NO_MODULE)

check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
test_large_files(HAVE_LARGEFILES)
check_header_stdc()
check_file_offset_bits()

if(MSVC)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(_WIN32_IE "0x0A00")
  else()
    set(_WIN32_IE "0x0600")
  endif()
  set(UNUSED " ")
else()
  set(UNUSED "__attribute__((unused))")
endif()

set(LT_OBJDIR "./libs")
set(PACKAGE "lib${PROJECT_NAME}")
set(PACKAGE_NAME ${PACKAGE})
set(PACKAGE_STRING "${PACKAGE} ${DVDNAV_VERSION}")
set(PACKAGE_TARNAME ${PACKAGE})
set(PACKAGE_URL "https://www.videolan.org/developers/libdvdnav.html")
set(PACKAGE_VERSION ${DVDNAV_VERSION})
set(STDC_HEADERS 1)
set(SUPPORT_ATTRIBUTE_VISIBILITY_DEFAULT 1)
set(SUPPORT_FLAG_VISIBILITY 1)
set(VERSION ${DVDNAV_VERSION})

set(DVDNAV_MAJOR ${DVDNAV_VERSION_MAJOR})
set(DVDNAV_MINOR ${DVDNAV_VERSION_MAJOR})
set(DVDNAV_SUB ${DVDNAV_VERSION_MICRO})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cm ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/dvdnav/version.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdnav/version.h @ONLY)

add_library(${PROJECT_NAME} SHARED
	src/dvdnav.c
	src/read_cache.c
	src/navigation.c
	src/highlight.c
  src/logger.c
  src/logger.h
	src/searching.c
	src/settings.c
	src/dvdnav_internal.h
	src/read_cache.h
	src/vm/decoder.c
	src/vm/decoder.h
	src/vm/vm.c
	src/vm/vm.h
	src/vm/play.c
	src/vm/play.h
	src/vm/getset.c
	src/vm/getset.h
	src/vm/vmget.c
	src/vm/vmcmd.c
	src/vm/vmcmd.h
)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
  PRIVATE
  $<BUILD_INTERFACE:.;src;src/dvdnav;msvc/include;msvc/contrib/dirent;${CMAKE_CURRENT_BINARY_DIR}>
  INTERFACE
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PRIVATE dvdread::dvdread)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      HAVE_CONFIG_H
)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "lib${PROJECT_NAME}")
if(MSVC)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _XBMC
      _CRT_SECURE_NO_WARNINGS
      _CRT_NONSTDC_NO_DEPRECATE
      PATH_MAX=1024
  )
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/msvc/libdvdnav.def\"")
  if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
     # fix linkage with dvdcss
     set_target_properties(${PROJECT_NAME}
       PROPERTIES
         LINK_FLAGS_DEBUG "/defaultlib:vccorlibd.lib /defaultlib:msvcrtd.lib"
         LINK_FLAGS_RELEASE "/defaultlib:vccorlib.lib /defaultlib:msvcrt.lib"
         LINK_FLAGS_RELWITHDEBINFO "/defaultlib:vccorlib.lib /defaultlib:msvcrt.lib"
    )
  endif()
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/dvdnav-config-version.cmake
  VERSION ${DVDNAV_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  CONFIGURATIONS Release RelWithDebInfo Debug)


install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdnav/dvd_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdnav/dvdnav.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdnav/dvdnav_events.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdnav/version.h
  DESTINATION include/dvdnav)

install(EXPORT ${PROJECT_NAME}
  FILE
    dvdnav.cmake
  NAMESPACE
    dvdnav::
  DESTINATION
    lib/cmake/dvdnav
)
install(
  FILES
    cmake/dvdnav-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/dvdnav-config-version.cmake
  DESTINATION
    lib/cmake/dvdnav
)
if(MSVC)
  install(FILES
    $<TARGET_PDB_FILE:dvdnav>
    DESTINATION lib
    CONFIGURATIONS RelWithDebInfo Debug
  )
endif()
export(TARGETS ${PROJECT_NAME}
  FILE
  ${CMAKE_CURRENT_BINARY_DIR}/dvdnav-config.cmake
  NAMESPACE dvdnav::
)
