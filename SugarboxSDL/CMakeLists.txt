cmake_minimum_required(VERSION 3.16)

project(SugarboxSDL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL3, built with the project (static)
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
   SDL3
   GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
   GIT_TAG release-3.4.18
   GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

# Dear ImGui (menus, status bar, dialogs), with its SDL3 and SDL_GPU backends. The docking branch :
# the debugger windows will need it.
FetchContent_Declare(
   imgui
   GIT_REPOSITORY https://github.com/ocornut/imgui.git
   GIT_TAG v1.92.9-docking
   GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
   ${imgui_SOURCE_DIR}/imgui.cpp
   ${imgui_SOURCE_DIR}/imgui_draw.cpp
   ${imgui_SOURCE_DIR}/imgui_tables.cpp
   ${imgui_SOURCE_DIR}/imgui_widgets.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# Frontend sources shared with the Qt application : no Qt in them
set(SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Sugarbox)
set(SHARED_SRCS
   ${SHARED_DIR}/ConfigurationManager.cpp
   ${SHARED_DIR}/Emulation.cpp
   ${SHARED_DIR}/ImageWriter.cpp
   ${SHARED_DIR}/Log.cpp
   ${SHARED_DIR}/SCLPlayer.cpp
   ${SHARED_DIR}/Script.cpp
   ${SHARED_DIR}/ScriptContext.cpp
   ${SHARED_DIR}/Z80Desassember.cpp
   ${SHARED_DIR}/debugers/DebugCommand.cpp
   ${SHARED_DIR}/debugers/DebugServer.cpp
)

set(SRCS
   main.cpp
   KeyboardMap.cpp
   KeyboardMap.h
   SDLDisplay.cpp
   SDLDisplay.h
   SDLSound.cpp
   SDLSound.h
   UserInterface.cpp
   UserInterface.h
)

add_executable(SugarboxSDL WIN32 ${SRCS} ${SHARED_SRCS})
source_group("Shared" FILES ${SHARED_SRCS})

target_include_directories(SugarboxSDL PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${SHARED_DIR}
   ${SHARED_DIR}/../json/single_include/nlohmann
)

if(UNIX)
   if(APPLE)
      target_compile_definitions(SugarboxSDL PRIVATE APPLE UNIX)
   else()
      target_compile_definitions(SugarboxSDL PRIVATE LINUX UNIX)
   endif()
elseif(WIN32)
   target_compile_definitions(SugarboxSDL PRIVATE WIN32 MINGW32 WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(MSVC)
   # The sources are UTF-8, as the Qt build reads them
   target_compile_options(SugarboxSDL PRIVATE /utf-8 /permissive-)
endif()

target_link_libraries(SugarboxSDL PRIVATE CPCCoreEmu zlibstatic imgui SDL3::SDL3-static)
if(WIN32)
   # timeBeginPeriod : 1 ms timer resolution, for the frame pacing. Sockets : debug server.
   target_link_libraries(SugarboxSDL PRIVATE winmm ws2_32)
elseif(CMAKE_COMPILER_IS_GNUCXX)
   target_link_libraries(SugarboxSDL PRIVATE pthread)
endif()

# Runs next to the Qt application : same ROM, CONF, Keyboards and Sugarbox.ini
install(TARGETS SugarboxSDL RUNTIME DESTINATION . BUNDLE DESTINATION .)
set_property(TARGET SugarboxSDL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
