cmake_minimum_required(VERSION 3.0)

# Project name
project(Sugarbox)

set(CMAKE_INSTALL_PREFIX libs)

add_subdirectory(CPCCore)
add_subdirectory(glfw)

#no config util
set ( ALSOFT_NO_CONFIG_UTIL TRUE)
set ( ALSOFT_EXAMPLES FALSE)
set ( ALSOFT_TESTS FALSE)
set ( ALSOFT_UTILS FALSE)

add_subdirectory(openal-soft)

set (IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

add_subdirectory(Sugarbox)
