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

set (IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../imgui)
set(GLAD_GL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../glfw")
            
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/examples ${GLAD_GL_DIR}/deps ${IMGUI_DIR}/examples/libs/gl3w)

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
   )



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

if(CMAKE_COMPILER_IS_GNUCC)
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
   set ( LIBZA libz.a stdc++fs )
endif()

set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GL3W")


add_executable(Sugarbox ${SRCS} ${HEADERS}  )

# Link test executable against gtest & gtest_main
target_link_libraries(Sugarbox CPCCoreEmu zlibstatic ${LIBZA} glfw opengl32)

# CPACK
set(CPACK_PACKAGE_NAME "Sugarbox")
set(CPACK_PACKAGE_VENDOR "Thomas GUILLEMIN")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sugarbox - An amstrad CPC Emulator")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sugarbox")

INCLUDE(CPack)
