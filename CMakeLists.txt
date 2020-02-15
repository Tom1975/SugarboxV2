cmake_minimum_required(VERSION 3.0)

# Project name
project(Sugarbox)

set(CMAKE_INSTALL_PREFIX libs)

#no config util
set ( ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "")
set ( ALSOFT_EXAMPLES OFF CACHE BOOL "")
set ( ALSOFT_TESTS OFF CACHE BOOL "")
set ( ALSOFT_UTILS OFF CACHE BOOL "")
set ( ALSOFT_CONFIG OFF CACHE BOOL "")
set ( ALSOFT_INSTALL OFF CACHE BOOL "")
set ( ALSOFT_HRTF_DEFS OFF CACHE BOOL "")
set ( ALSOFT_AMBDEC_PRESETS OFF CACHE BOOL "")

set ( FORCE_STATIC_VCRT ON CACHE BOOL "" FORCE )
set ( BUILD_SHARED_LIB_ OFF CACHE BOOL "" FORCE )
set ( USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE )

add_subdirectory(CPCCore)
add_subdirectory(glfw)
add_subdirectory(openal-soft)

set (IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
add_subdirectory(Sugarbox)
