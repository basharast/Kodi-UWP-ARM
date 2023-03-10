set(SOURCES d3dxGlobal.cpp
            EffectAPI.cpp
            EffectLoad.cpp
            EffectNonRuntime.cpp
            EffectReflection.cpp
            EffectRuntime.cpp)

add_library(effects11 STATIC ${SOURCES})
target_include_directories(effects11
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Binary
)

target_compile_options(effects11 PRIVATE "/permissive")

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  target_compile_definitions(effects11 PRIVATE
    WIN32
    _WINDOWS
    _LIB
    D3DXFX_LARGEADDRESS_HANDLE
    _WIN32_WINNT=0x0A00
    _CRT_STDIO_ARBITRARY_WIDE_SPECIFIERS
  )
else()
target_compile_definitions(effects11 PRIVATE
  WIN32
  _WINDOWS
  _LIB
  D3DXFX_LARGEADDRESS_HANDLE
  _CRT_STDIO_ARBITRARY_WIDE_SPECIFIERS
)
endif()

add_precompiled_header(effects11 pchfx.h d3dxGlobal.cpp)
set_target_properties(effects11 PROPERTIES FOLDER lib)
source_group_by_folder(effects11)
set(core_DEPENDS effects11 ${core_DEPENDS} CACHE STRING "" FORCE)

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  set_target_properties(effects11 PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
endif()
