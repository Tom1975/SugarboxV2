cmake_minimum_required(VERSION 3.0)

# Project name
project(Sugarbox)

set(CMAKE_INSTALL_PREFIX libs)

add_subdirectory(CPCCore)
add_subdirectory(glfw)

#no config util
set ( ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "")
set ( ALSOFT_EXAMPLES OFF CACHE BOOL "")
set ( ALSOFT_TESTS OFF CACHE BOOL "")
set ( ALSOFT_UTILS OFF CACHE BOOL "")
set ( ALSOFT_CONFIG OFF CACHE BOOL "")
set ( ALSOFT_INSTALL OFF CACHE BOOL "")
set ( ALSOFT_HRTF_DEFS OFF CACHE BOOL "")
set ( ALSOFT_AMBDEC_PRESETS OFF CACHE BOOL "")

add_subdirectory(openal-soft)

set (IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

add_subdirectory(Sugarbox)
