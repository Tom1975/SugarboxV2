cmake_minimum_required(VERSION 3.0)
project(Sugarbox)

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)


# Turn on automatic invocation of the MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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

find_package(Qt5 COMPONENTS Widgets REQUIRED)
qt5_add_resources(QRCS resources.qrc)

file(GLOB HEADERS
    *.h
)

file(GLOB SRCS
    *.cpp
)

file(GLOB RES
   ${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*
)
            
set (HEADERS 
   ${HEADERS}
)
set (SRCS 
      ${SRCS}
   )

set ( UIS
 SugarboxApp.ui
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
if(CMAKE_COMPILER_IS_GNUCC)
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++17 )
   set ( LIB_EXT pthread stdc++fs)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++17 )
   set ( LIBZA libz.a pthread stdc++fs)
endif()

IF(WIN32)
   set (OPENGL_LINK opengl32)
endif()

add_executable(Sugarbox ${SRCS} ${HEADERS} ${QRCS} ${UIS} ${RES}  )

# Link test executable against gtest & gtest_main
target_link_libraries(Sugarbox CPCCoreEmu zlibstatic OpenAL Qt5::Core Qt5::Widgets ${LIB_EXT})

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
set(CPACK_GENERATOR "TGZ;ZIP")
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

