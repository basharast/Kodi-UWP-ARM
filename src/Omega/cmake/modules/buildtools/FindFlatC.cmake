# FindFlatC
# --------
# Find the FlatBuffers schema compiler
#
# This will define the following target:
#
#   flatbuffers::flatc - The FlatC compiler

if(NOT TARGET flatbuffers::flatc)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  # Check for existing FLATC.
  find_program(FLATBUFFERS_FLATC_EXECUTABLE NAMES flatc
                                            HINTS ${NATIVEPREFIX}/bin
                                            NO_CACHE)

  if(FLATBUFFERS_FLATC_EXECUTABLE)
    execute_process(COMMAND "${FLATBUFFERS_FLATC_EXECUTABLE}" --version
                    OUTPUT_VARIABLE FLATBUFFERS_FLATC_VERSION
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "[^\n]* version [^\n]*" FLATBUFFERS_FLATC_VERSION "${FLATBUFFERS_FLATC_VERSION}")
    string(REGEX REPLACE ".* version (.*)" "\\1" FLATBUFFERS_FLATC_VERSION "${FLATBUFFERS_FLATC_VERSION}")
  endif()

  set(MODULE_LC flatbuffers)
  set(LIB_TYPE native)
  # Duplicate URL may exist from FindFlatbuffers.cmake
  # unset otherwise it thinks we are providing a local file location and incorrect concatenation happens
  unset(FLATBUFFERS_URL)
  SETUP_BUILD_VARS()

  if(NOT FLATBUFFERS_FLATC_EXECUTABLE OR
      (ENABLE_INTERNAL_FLATBUFFERS AND NOT "${FLATBUFFERS_FLATC_VERSION}" VERSION_EQUAL "${FLATBUFFERS_VER}"))

    # Override build type detection and always build as release
    set(FLATBUFFERS_BUILD_TYPE Release)

    if(NATIVEPREFIX)
      set(INSTALL_DIR "${NATIVEPREFIX}/bin")
      set(FLATBUFFERS_INSTALL_PREFIX ${NATIVEPREFIX})
    else()
      set(INSTALL_DIR "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin")
      set(FLATBUFFERS_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
    endif()

    set(CMAKE_ARGS -DFLATBUFFERS_CODE_COVERAGE=OFF
                   -DFLATBUFFERS_BUILD_TESTS=OFF
                   -DFLATBUFFERS_INSTALL=ON
                   -DFLATBUFFERS_BUILD_FLATLIB=OFF
                   -DFLATBUFFERS_BUILD_FLATC=ON
                   -DFLATBUFFERS_BUILD_FLATHASH=OFF
                   -DFLATBUFFERS_BUILD_GRPCTEST=OFF
                   -DFLATBUFFERS_BUILD_SHAREDLIB=OFF)

    # Set host build info for buildtool
    if(EXISTS "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
      set(FLATBUFFERS_TOOLCHAIN_FILE "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
    endif()

    if(WIN32 OR WINDOWS_STORE)
      # Make sure we generate for host arch, not target
      set(FLATBUFFERS_GENERATOR_PLATFORM CMAKE_GENERATOR_PLATFORM ${HOSTTOOLSET})
      set(WIN_DISABLE_PROJECT_FLAGS 1)
    endif()

    set(FLATBUFFERS_FLATC_EXECUTABLE ${INSTALL_DIR}/flatc)

    set(BUILD_NAME flatc)
    set(BUILD_BYPRODUCTS ${FLATBUFFERS_FLATC_EXECUTABLE})
    set(FLATBUFFERS_FLATC_VERSION ${FLATBUFFERS_VER})

    BUILD_DEP_TARGET()
  endif()

  include(FindPackageMessage)
  find_package_message(FlatC "Found FlatC Compiler: ${FLATBUFFERS_FLATC_EXECUTABLE} (found version \"${FLATBUFFERS_FLATC_VERSION}\")" "[${FLATBUFFERS_FLATC_EXECUTABLE}][${FLATBUFFERS_FLATC_VERSION}]")

  add_executable(flatbuffers::flatc IMPORTED)
  set_target_properties(flatbuffers::flatc PROPERTIES
                                           IMPORTED_LOCATION "${FLATBUFFERS_FLATC_EXECUTABLE}"
                                           FOLDER "External Projects")

  if(TARGET flatc)
    add_dependencies(flatbuffers::flatc flatc)
  endif()
endif()
