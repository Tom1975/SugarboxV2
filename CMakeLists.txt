cmake_minimum_required(VERSION 3.0)

# Project name
project(Sugarbox)

set (USE_SFML TRUE)

# Static wxWidget runtime
set (wxBUILD_USE_STATIC_RUNTIME TRUE)

if (MSVC)
   MESSAGE("Force Unicode")
   set (wxUSE_UNICODE TRUE)
else()
   MESSAGE("Force Unicode as FALSE")
   set (wxUSE_UNICODE FALSE)
endif()
set(CMAKE_INSTALL_PREFIX libs)

add_subdirectory(CPCCore)
add_subdirectory(wxWidgets)


add_subdirectory(Sugarbox)


