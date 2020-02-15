cmake_minimum_required(VERSION 3.0)

# Project name
project(Sugarbox)

set(CMAKE_INSTALL_PREFIX libs)

add_subdirectory(CPCCore)
add_subdirectory(glfw)

#no config util
set ( ALSOFT_NO_CONFIG_UTIL ON)
set ( ALSOFT_EXAMPLES OFF)
set ( ALSOFT_TESTS OFF)
set ( ALSOFT_UTILS OFF)
set ( ALSOFT_CONFIG OFF)
set ( ALSOFT_INSTALL OFF)
set ( ALSOFT_HRTF_DEFS OFF)
set ( ALSOFT_AMBDEC_PRESETS OFF )

add_subdirectory(openal-soft)

set (IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

add_subdirectory(Sugarbox)
