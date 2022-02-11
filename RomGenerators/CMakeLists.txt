cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)
# setting this is required

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(
  CordicRomGenerator
  LANGUAGES CXX
  VERSION 0.1)

add_library(romgen sources/RomGeneratorMCHalfPi/RomGeneratorMCHalfPi.cpp
                   sources/RomGeneratorConst/RomGeneratorConst.cpp)
target_include_directories(romgen PUBLIC sources)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL
                                        "RelWithDebInfo")
  target_compile_definitions(romgen PRIVATE DEBUG=1)
else()
  target_compile_definitions(romgen PRIVATE NDEBUG=1)
endif()

add_executable(rom_generator sources/main.cpp)
target_link_libraries(rom_generator PUBLIC romgen)
