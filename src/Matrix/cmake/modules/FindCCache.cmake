#.rst:
# FindCCache
# ----------
# Finds ccache and sets it up as compiler wrapper.
# This should ideally be called before the call to project().
#
# See: https://crascit.com/2016/04/09/using-ccache-with-cmake/

find_program(CCACHE_PROGRAM ccache)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CCache REQUIRED_VARS CCACHE_PROGRAM)

if(CCACHE_FOUND)
  # Supports Unix Makefiles, Ninja and Xcode
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")

  file(WRITE "${CMAKE_BINARY_DIR}/launch-c" "#!/bin/sh\nexec \"${CCACHE_PROGRAM}\" \"${CMAKE_C_COMPILER}\" \"$@\"\n")
  file(WRITE "${CMAKE_BINARY_DIR}/launch-cxx" "#!/bin/sh\nexec \"${CCACHE_PROGRAM}\" \"${CMAKE_CXX_COMPILER}\" \"$@\"\n")
  execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/launch-c" "${CMAKE_BINARY_DIR}/launch-cxx")

  set(CMAKE_XCODE_ATTRIBUTE_CC "${CMAKE_BINARY_DIR}/launch-c" PARENT_SCOPE)
  set(CMAKE_XCODE_ATTRIBUTE_CXX "${CMAKE_BINARY_DIR}/launch-cxx" PARENT_SCOPE)
  set(CMAKE_XCODE_ATTRIBUTE_LD "${CMAKE_BINARY_DIR}/launch-c" PARENT_SCOPE)
  set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx" PARENT_SCOPE)
endif()
