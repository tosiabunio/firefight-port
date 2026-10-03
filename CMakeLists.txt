cmake_minimum_required(VERSION 3.25)

project(FireFight
  VERSION 0.1.0
  DESCRIPTION "Port of Fire Fight (Chaos Works, 1996) to SDL2"
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Fire Fight is built as 64-bit only (see docs/porting-plan.md).")
endif()

option(FF_BUILD_GAME "Build the game (compat, cwengine, regdata, firefight)" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FFCompileOptions)
include(FFDependencies)
include(CTest)

# Repository data used at run time and by tests.
set(FF_DATA_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(FF_MUSIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/music")

add_subdirectory(tools/smoke)

if(FF_BUILD_GAME)
  add_subdirectory(source)
endif()
