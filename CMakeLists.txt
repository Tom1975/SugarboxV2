cmake_minimum_required(VERSION 3.0)

# Project name
project(Sugarbox)

set (USE_SFML TRUE)

# Static wxWidget runtime
set (wxBUILD_USE_STATIC_RUNTIME TRUE)

add_subdirectory(CPCCore)
add_subdirectory(wxWidgets)
add_subdirectory(Sugarbox)


