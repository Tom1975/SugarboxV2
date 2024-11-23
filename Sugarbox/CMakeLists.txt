IF(MSVC)
   cmake_minimum_required(VERSION 3.14)
else()
   cmake_minimum_required(VERSION 3.10)
endif()

project(Sugarbox)

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)

# Turn on automatic invocation of the MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set (SUGARBOX_VERSION "2.0.3" CACHE STRING "Version to build" FORCE)

MESSAGE (STATUS "Version :" ${SUGARBOX_VERSION})

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

find_package(Qt6 COMPONENTS Core Widgets WebSockets OpenGLWidgets OpenGL REQUIRED)
qt6_add_resources(QRCS resources.qrc)

if(Qt6_FOUND AND WIN32 AND TARGET Qt6::qmake AND NOT TARGET Qt6::windeployqt)
    get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)

    execute_process(
        COMMAND "${_qt6_qmake_location}" -query QT_INSTALL_PREFIX
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt6_install_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(imported_location "${qt6_install_prefix}/bin/windeployqt.exe")

    if(EXISTS ${imported_location})
        add_executable(Qt6::windeployqt IMPORTED)

        set_target_properties(Qt6::windeployqt PROPERTIES
            IMPORTED_LOCATION ${imported_location}
        )
    endif()
endif()

get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

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

set (EXTRA
    ../changelog.txt
    ../LICENSE
    ../README.md
   )

set ( UIS
 SugarboxApp.ui
 DebugDialog.ui
 MemoryDialog.ui
)

set (LABELS
   Resources/labels.ini
)
set (ACTION
    ../.github/workflows/macos.yml
    ../.github/workflows/release.yml
    ../.github/workflows/ubuntu.yml
    ../.github/workflows/windows.yml
   )

source_group("Resources" FILES ${RES} )
source_group("Docs" FILES ${EXTRA} )
source_group("Github Action" FILES ${ACTION} )
source_group("Labels" FILES ${LABELS} )


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

IF(APPLE)
   add_executable(Sugarbox MACOSX_BUNDLE ${EXTRA_DEF} ${SRCS} ${HEADERS} ${QRCS} ${UIS} ${RES} ${EXTRA} ${ACTION} ${LABELS}} )
else()
   add_executable(Sugarbox ${EXTRA_DEF} ${SRCS} ${HEADERS} ${QRCS} ${UIS} ${RES} ${EXTRA} ${ACTION} ${LABELS})
endif()



IF(APPLE)
   MESSAGE (STATUS "MACOS Specific build")
   set_target_properties(Sugarbox PROPERTIES  MACOSX_BUNDLE ON    )
   set_target_properties(Sugarbox PROPERTIES
       BUNDLE True
       MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.Sugarbox
       MACOSX_BUNDLE_BUNDLE_NAME Sugarbox
       MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
       MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
       MACOSX_BUNDLE_COPYRIGHT "Thomas GUILLEMIN"
   )
 
   target_link_libraries(Sugarbox "-framework OpenGL")

endif()

target_compile_definitions(Sugarbox PRIVATE SUGARBOX_VERSION=${SUGARBOX_VERSION} )

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_INSTALL_BINDIR})
set ( SUGARBOX_OUTPUT_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})

# Link test executable against gtest & gtest_main
target_link_libraries(Sugarbox CPCCoreEmu zlibstatic OpenAL Qt6::Core Qt6::Widgets Qt6::WebSockets Qt6::OpenGLWidgets  ${LIB_EXT})

set_property(TARGET Sugarbox PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_INSTALL_BINDIR}")
MESSAGE ( CMAKE_INSTALL_BINDIR = ${CMAKE_INSTALL_BINDIR})

# Ready-to-use configuration
if(WIN32)
   install (CODE [[
   if($<CONFIG:Release>)
    set (__qt_deploy_tool_extra_options "--release")
   else()
      if($<CONFIG:RelWithDebInfo> )
         set (__qt_deploy_tool_extra_options "--release")
      else()
      MESSAGE (STATUS \"Nothing to go\")
      endif()
   endif()
   ]]
   )
endif()

install ( TARGETS Sugarbox RUNTIME DESTINATION . BUNDLE DESTINATION .)
install ( FILES "${PROJECT_SOURCE_DIR}/Sugarbox.ini" DESTINATION .) 
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ROM" DESTINATION .) 
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CART" DESTINATION .) 
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CONF" DESTINATION .) 
install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Resources" DESTINATION .) 

IF(APPLE)
   set(deploy_tool_options_arg --hardened-runtime)
elseif(WIN32)
endif()

qt_generate_deploy_app_script(
    TARGET Sugarbox
	 FILENAME_VARIABLE deploy_script
    #OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    #NO_COMPILER_RUNTIME
	)

install(SCRIPT ${deploy_script})

set_property(TARGET Sugarbox PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")




# CPACK
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_NAME "Sugarbox")
set(CPACK_PACKAGE_VENDOR "Thomas GUILLEMIN")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sugarbox - An Amstrad CPC/CPC+/GX4000 Emulator")
set(CPACK_PACKAGE_VERSION ${SUGARBOX_VERSION} )
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sugarbox")

INCLUDE(CPack)
