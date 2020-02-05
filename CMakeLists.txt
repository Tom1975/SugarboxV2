cmake_minimum_required(VERSION 3.0)

# Project name
project(Sugarbox)

set (USE_SFML TRUE)

set(CMAKE_INSTALL_PREFIX libs)


add_subdirectory(CPCCore)

set (IMGUI_SFML_FIND_SFML FALSE)
set (SFML_DIR ${CMAKE_BINARY_DIR}/CPCCore/SFML)
set (IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

MESSAGE ("IMGUI_SFML_FIND_SFML  : ${IMGUI_SFML_FIND_SFML}")
MESSAGE ("SFML_DIR  : ${SFML_DIR}")
MESSAGE ("IMGUI_DIR  : ${IMGUI_DIR}")

add_subdirectory(glfw)
#add_subdirectory(openal-soft)
add_subdirectory(Sugarbox)


