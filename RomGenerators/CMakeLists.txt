#
# Copyright 2022 Camille "DrasLorus" Monière.
#
# This file is part of CORDIC_Rotate_APFX.
#
# This program is free software: you can redistribute it and/or modify it under the terms of the GNU
# Lesser General Public License as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with this program.
# If not, see <https://www.gnu.org/licenses/>.
#

cmake_minimum_required (VERSION 3.16.0 FATAL_ERROR)
# setting this is required

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set (CMAKE_EXPORT_COMPILE_COMMANDS true)

project (
  CordicRomGenerator
  LANGUAGES CXX
  VERSION 0.1
)

add_library (
  romgen sources/RomGeneratorML/RomGeneratorML.cpp sources/RomGeneratorConst/RomGeneratorConst.cpp
)
target_include_directories (romgen PUBLIC sources)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  target_compile_definitions (romgen PRIVATE DEBUG=1)
else ()
  target_compile_definitions (romgen PRIVATE NDEBUG=1)
endif ()

add_executable (rom_generator_legacy sources/main_legacy.cpp)
target_link_libraries (rom_generator_legacy PUBLIC romgen)

set (
  ROM_TYPE
  "ml"
  CACHE STRING "RomGenerator to use, either 'ml' or 'const'."
)
set (
  CORDIC_W
  "16"
  CACHE STRING "bit length of the CORDIC input."
)
set (
  CORDIC_STAGES
  "6"
  CACHE STRING "number of CORDIC stages."
)
set (
  CORDIC_Q
  "64"
  CACHE STRING "number of rotation divisions."
)
set (
  CORDIC_DIVIDER
  "2"
  CACHE STRING "Rotation denominator."
)

set (
  current_generator_source
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/main_generator_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}_${CORDIC_DIVIDER}.cpp
)
configure_file (sources/main_generator.cpp.in ${current_generator_source} @ONLY)
add_executable (rom_generator ${current_generator_source})
target_link_libraries (rom_generator PUBLIC romgen)
