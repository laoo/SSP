cmake_minimum_required (VERSION 3.12)

project( SSP )
set( CMAKE_CXX_STANDARD 20 )

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(Boost_USE_STATIC_LIBS ON) 
find_package(Boost 1.77.0 COMPONENTS program_options) 

include_directories( ${Boost_INCLUDE_DIRS} )
include_directories( extern )

add_executable( SSP
  SuzySpritePacker.cpp
  ProgramOptions.cpp
  Palette.cpp
  Image.cpp
  ImageRow.cpp
  Sprite.cpp
  Log.cpp
  extern/lodepng.cpp
)

target_link_libraries( SSP
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

