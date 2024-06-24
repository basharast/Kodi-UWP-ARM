cmake_minimum_required(VERSION 3.1)

set(DVDREAD_VERSION_MAJOR 6)
set(DVDREAD_VERSION_MINOR 1)
set(DVDREAD_VERSION_MICRO 3)
set(DVDREAD_VERSION "${DVDREAD_VERSION_MAJOR}.${DVDREAD_VERSION_MINOR}.${DVDREAD_VERSION_MICRO}")
set(PACKAGE_VERSION ${DVDREAD_VERSION})

project(dvdread LANGUAGES C VERSION ${DVDREAD_VERSION})

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
if(dvdcss_FOUND)
  set(HAVE_DVDCSS_DVDCSS_H 1)
endif()

check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
test_large_files(HAVE_LARGEFILES)
check_header_stdc()
check_file_offset_bits()

if(MSVC)
  if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    add_definitions(/wd4146)
  endif()
endif()

set(LT_OBJDIR "./libs")
set(PACKAGE ${PROJECT_NAME})
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_STRING "${PROJECT_NAME} ${DVDREAD_VERSION}")
set(PACKAGE_TARNAME ${PROJECT_NAME})
set(PACKAGE_URL "https://www.videolan.org/developers/libdvdread.html")
set(PACKAGE_VERSION ${DVDREAD_VERSION})
set(STDC_HEADERS 1)
set(SUPPORT_ATTRIBUTE_VISIBILITY_DEFAULT 1)
set(SUPPORT_FLAG_VISIBILITY 1)
set(VERSION ${DVDREAD_VERSION})
set(_WIN32_IE "0x0600")
#set(UNUSED "__attribute__((unused))")
set(UNUSED " ")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cm ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/version.h.in ${CMAKE_BINARY_DIR}/version.h @ONLY)

add_library(${PROJECT_NAME}
  src/bitreader.c
  src/bswap.h
  src/dvd_input.c
  src/dvd_input.h
  src/dvd_reader.c
  src/dvd_udf.c
  src/dvdread_internal.h
  src/ifo_print.c
  src/ifo_read.c
  src/md5.c
  src/md5.h
  src/nav_print.c
  src/nav_read.c
  src/logger.c
  src/logger.h
)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
  PRIVATE
  $<BUILD_INTERFACE:.;src;src/dvdread;msvc/include;${CMAKE_CURRENT_BINARY_DIR}>
  INTERFACE
  $<INSTALL_INTERFACE:include>
)
if(dvdcss_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE dvdcss::dvdcss)
endif()

if(MSVC)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE 
      _XBMC
      _CRT_SECURE_NO_WARNINGS
      _CRT_NONSTDC_NO_DEPRECATE
      PATH_MAX=1024
      WIN32_LEAN_AND_MEAN
  )
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/dvdread-config-version.cmake
  VERSION ${DVDREAD_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  CONFIGURATIONS RelWithDebInfo Debug
)


install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/bitreader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/dvd_reader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/dvd_udf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/ifo_print.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/ifo_read.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/ifo_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/nav_print.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/nav_read.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dvdread/nav_types.h
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
  DESTINATION include/dvdread)

install(EXPORT ${PROJECT_NAME}
  FILE
    dvdread.cmake
  NAMESPACE
    dvdread::
  DESTINATION
    lib/cmake/dvdread
)
install(
  FILES
    cmake/dvdread-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/dvdread-config-version.cmake
  DESTINATION
    lib/cmake/dvdread
)
if(MSVC AND BUILD_SHARED_LIBS)
  install(FILES 
    $<TARGET_PDB_FILE:dvdread>
    DESTINATION lib
    CONFIGURATIONS RelWithDebInfo Debug
  )
endif()
export(TARGETS dvdread
  FILE
  ${CMAKE_CURRENT_BINARY_DIR}/dvdread-config.cmake
  NAMESPACE dvdread::
)
