cmake_minimum_required(VERSION 3.14)
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

if(Qt5_FOUND AND WIN32 AND TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
    get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

    execute_process(
        COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt5_install_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")

    if(EXISTS ${imported_location})
        add_executable(Qt5::windeployqt IMPORTED)

        set_target_properties(Qt5::windeployqt PROPERTIES
            IMPORTED_LOCATION ${imported_location}
        )
    endif()
endif()

get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

function(windeployqt target)

    # POST_BUILD step
    # - after build, we have a bin/lib for analyzing qt dependencies
    # - we run windeployqt on target and deploy Qt libs

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${_qt_bin_dir}/windeployqt.exe"         
                \"$<TARGET_FILE:${target}>\"
        COMMENT "Deploying Qt libraries using windeployqt for compilation target '${target}' ..."
    )

endfunction()

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


set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set ( SUGARBOX_OUTPUT_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
MESSAGE ("SUGARBOX_OUTPUT_DEBUG" ${SUGARBOX_OUTPUT_DEBUG})

# Link test executable against gtest & gtest_main
target_link_libraries(Sugarbox CPCCoreEmu zlibstatic OpenAL Qt5::Core Qt5::Widgets ${LIB_EXT})
set_property(TARGET Sugarbox PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${SUGARBOX_OUTPUT_DEBUG}")
#if(TARGET Qt5::windeployqt)
    # execute windeployqt in a tmp directory after build
#    add_custom_command(TARGET Sugarbox
#        POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
#        COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
#        COMMAND Qt5::windeployqt --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:Sugarbox>/$<TARGET_FILE_NAME:Sugarbox>"
#    )
#
#    # copy deployment directory during installation
#    install(
#        DIRECTORY
#        "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
#        CONFIGURATIONS Debug 
#        DESTINATION ${SUGARBOX_OUTPUT_DEBUG}
#    )
#endif()

if (APPLE)
  target_link_libraries(Sugarbox "-framework OpenGL")
endif()

# Installation
install ( TARGETS Sugarbox RUNTIME DESTINATION . )
install ( FILES "${PROJECT_SOURCE_DIR}/Sugarbox.ini" DESTINATION . ) 

# Ready-to-use configuration
foreach(loop_var ${CMAKE_CONFIGURATION_TYPES})
   MESSAGE ( "loop_var  = " ${loop_var})
   install ( FILES "${PROJECT_SOURCE_DIR}/Sugarbox.ini" CONFIGURATIONS ${loop_var} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${loop_var} ) 
   install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ROM" CONFIGURATIONS ${loop_var} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${loop_var} ) 
   install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CART" CONFIGURATIONS ${loop_var} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${loop_var} ) 
   install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CONF" CONFIGURATIONS ${loop_var} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${loop_var} )  
   install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Resources" CONFIGURATIONS ${loop_var} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${loop_var} ) 
endforeach(loop_var)

# MSVC CPACK Qt dlls
IF(MSVC)
   install (CODE "
      set (_outpath ${CMAKE_CURRENT_BINARY_DIR}/$<$<CONFIG:debug>:Debug>$<$<CONFIG:release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>$<$<CONFIG:MinSizeRel>:MinSizeRel>/)
      set (_file ${CMAKE_CURRENT_BINARY_DIR}/$<$<CONFIG:debug>:Debug>$<$<CONFIG:release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>$<$<CONFIG:MinSizeRel>:MinSizeRel>/Sugarbox.exe)
      set ( rel_or_debug  $<$<CONFIG:debug>:--debug>$<$<CONFIG:release>:--release>$<$<CONFIG:RelWithDebInfo>:--release>$<$<CONFIG:MinSizeRel>:--release> )
      execute_process(
              COMMAND \"${CMAKE_COMMAND}\" -E
                  env PATH=\"${_qt_bin_dir}\" \"${WINDEPLOYQT_EXECUTABLE}\"
                      --dry-run
                      --no-compiler-runtime
                      --list mapping
                      \${rel_or_debug}
                      \${_file}
               OUTPUT_VARIABLE _output
               OUTPUT_STRIP_TRAILING_WHITESPACE
          )
      separate_arguments(_files WINDOWS_COMMAND \${_output})
      while(_files)
         list(GET _files 0 _src)
         list(GET _files 1 _dest)
         # This is for install
         execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E
            copy   \${_src} \"\${_outpath}/\${_dest}\"
         )
         # This is for cpack
         execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E
            copy   \${_src} \"\${CMAKE_INSTALL_PREFIX}/\${_dest}\"
         )
         list(REMOVE_AT _files 0 1)
      endwhile()
   "  )
endif()

# CPACK
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_NAME "Sugarbox")
set(CPACK_PACKAGE_VENDOR "Thomas GUILLEMIN")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sugarbox - An Amstrad CPC/CPC+/GX4000 Emulator")
set(CPACK_PACKAGE_VERSION "1.99.1")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sugarbox")
set(CPACK_INSTALLED_DIRECTORIES 
   "${CMAKE_CURRENT_SOURCE_DIR}/Resources" "Resources"
   "${CMAKE_CURRENT_SOURCE_DIR}/ROM" "ROM"
   "${CMAKE_CURRENT_SOURCE_DIR}/CART" "CART"
   "${CMAKE_CURRENT_SOURCE_DIR}/CONF" "CONF")

INCLUDE(CPack)

# Install
