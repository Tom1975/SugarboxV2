cmake_minimum_required(VERSION 3.0)
project(Sugarbox)

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
		_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING 
        )

file(GLOB HEADERS
    *.h
)

file(GLOB SRCS
    *.cpp
)

file(GLOB RES
   ${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*
)

set (IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../imgui)
set (IMGUI_FILEDIALOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ImGuiFileDialog)
set(GLAD_GL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../glfw")
            
include_directories(${IMGUI_DIR} 
                    ${IMGUI_DIR}/examples 
                    ${GLAD_GL_DIR}/deps 
                    ${IMGUI_DIR}/examples/libs/gl3w 
                    ${IMGUI_FILEDIALOG_DIR}
                    )

set (HEADERS 
   ${HEADERS}
   ${IMGUI_DIR}/examples/imgui_impl_glfw.h
   ${IMGUI_DIR}/examples/imgui_impl_opengl3.h
)
set (SRCS 
      ${SRCS}
      ${IMGUI_DIR}/imgui.cpp
      ${IMGUI_DIR}/imgui_draw.cpp
      ${IMGUI_DIR}/imgui_widgets.cpp
      ${IMGUI_DIR}/examples/imgui_impl_glfw.cpp
      ${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp
      ${IMGUI_DIR}/examples/libs/gl3w/GL/gl3w.c
      ${IMGUI_FILEDIALOG_DIR}/ImGuiFileDialog/ImGuiFileDialog.cpp
   )

source_group("Resources" FILES ${RES} )

if(UNIX)
    if (APPLE)
        add_definitions(-DAPPLE)
    else()
        add_definitions(-DLINUX)
    endif()
    add_definitions(-DUNIX)
elseif(WIN32)
    add_definitions(-DWIN32)
    add_definitions(-DMINGW32)
    set(DIRENT_INCLUDE_DIR ${IMGUI_FILEDIALOG_DIR}/3rdparty/dirent/include)
    
    include_directories(${DIRENT_INCLUDE_DIR})
endif()

IF(MSVC)
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
   if (USE_SHARED_CRT)
   else()
      foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
      endforeach()
   endif()
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")  
ENDIF(MSVC)

if(CMAKE_COMPILER_IS_GNUCC )
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
   set ( LIBZA libz.a stdc++fs GLEW GL )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
   set ( LIBZA libz.a )
endif()

set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GL3W")
set ( CMAKE_CXX_STANDARD 17 )

IF(WIN32)
   set (OPENGL_LINK opengl32)
endif()

add_executable(Sugarbox ${SRCS} ${HEADERS} ${RES} )

# Link test executable against gtest & gtest_main
target_link_libraries(Sugarbox CPCCoreEmu zlibstatic ${LIBZA} glfw ${OPENGL_LINK} OpenAL)
if (APPLE)
  target_link_libraries(Sugarbox "-framework OpenGL")
endif()

# Installation
install ( TARGETS Sugarbox RUNTIME DESTINATION . )
install ( FILES "${PROJECT_SOURCE_DIR}/Sugarbox.ini" DESTINATION . ) 

# Ready-to-use debug_
install ( FILES "${PROJECT_SOURCE_DIR}/Sugarbox.ini" DESTINATION ${PROJECT_BINARY_DIR} ) 
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ROM" DESTINATION ${PROJECT_BINARY_DIR} ) 
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CART" DESTINATION ${PROJECT_BINARY_DIR} ) 
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CONF" DESTINATION ${PROJECT_BINARY_DIR} )  
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Resources" DESTINATION ${PROJECT_BINARY_DIR} ) 


# CPACK
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "Sugarbox")
set(CPACK_PACKAGE_VENDOR "Thomas GUILLEMIN")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sugarbox - An amstrad CPC Emulator")
set(CPACK_PACKAGE_VERSION "1.99.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "99")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sugarbox")
set(CPACK_INSTALLED_DIRECTORIES 
   "${CMAKE_CURRENT_SOURCE_DIR}/Resources" "Resources"
   "${CMAKE_CURRENT_SOURCE_DIR}/ROM" "ROM"
   "${CMAKE_CURRENT_SOURCE_DIR}/CART" "CART"
   "${CMAKE_CURRENT_SOURCE_DIR}/CONF" "CONF")

INCLUDE(CPack)

# Install

