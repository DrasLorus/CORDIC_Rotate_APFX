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
  CordicRotate
  LANGUAGES CXX
  VERSION 0.1
)

# ##################################################################################################
# Options
# ##################################################################################################

option (EXPORT_COMMANDS "export compile commands, for use with clangd for example." ON)
option (ENABLE_XILINX "use Xilinx provided proprietary headers." OFF)

option (ENABLE_TESTING "use Catch2 in conjunction with CTest as a test suite." ON)

option (PEDANTIC "use -Wall and -pedantic." ON)

# ##################################################################################################

if (PEDANTIC)
  add_compile_options (-Wall -pedantic)
endif ()

if (EXPORT_COMMANDS)
  set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()

if (ENABLE_XILINX)
  set (
    XILINX_HOME
    /opt/Xilinx
    CACHE PATH "path to Xilinx root folder."
  )
  set (
    XILINX_VER
    "2020.2"
    CACHE STRING "Xilinx software version to use."
  )

  if (XILINX_VER VERSION_GREATER_EQUAL "2020.1")
    set (AP_INCLUDE_DIR ${XILINX_HOME}/Vitis_HLS/${XILINX_VER}/include)
  else ()
    set (AP_INCLUDE_DIR ${XILINX_HOME}/Vivado/${XILINX_VER}/include)
  endif ()
  message (STATUS "AP headers must lie under ${AP_INCLUDE_DIR}")
else ()
  set (
    AP_TYPES_HINT
    /usr/include
    CACHE PATH "location of ap_types include directory."
  )
  find_file (
    AP_FIXED ap_fixed.h
    HINTS ${AP_TYPES_HINT}
    PATH_SUFFIXES ap_types hls_ap_types/include REQUIRED
  )

  get_filename_component (AP_INCLUDE_DIR ${AP_FIXED} DIRECTORY)
endif ()

if ((NOT EXISTS ${AP_INCLUDE_DIR}/ap_int.h) OR (NOT EXISTS ${AP_INCLUDE_DIR}/ap_fixed.h))
  message (FATAL_ERROR "Arbitrary precision headers not found in ${AP_INCLUDE_DIR}.\n"
                       "Please provide a suitable path to the headers."
  )
endif ()

set (
  ROM_TYPE
  "mc"
  CACHE STRING "RomGenerator to use, either 'mc' or 'cst'."
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

add_subdirectory (RomGenerators)

set (ROM_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sources/CordicRoms)
string (CONFIGURE ${ROM_DIRECTORY}/cordic_rom_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}.hpp ROM_HEADER
     
)
add_custom_command (
  OUTPUT ${ROM_HEADER}
  COMMAND rom_generator
  WORKING_DIRECTORY ${ROM_DIRECTORY}
)

set (CORDIC_ROM_HEADER CCordicRotateRom_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}.hpp)
set (CORDIC_ROM_SOURCE CCordicRotateRom_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}.cpp)
configure_file (
  sources/CCordicRotateRom/CCordicRotateRom.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/CCordicRotateRom/${CORDIC_ROM_HEADER} @ONLY
)
configure_file (
  sources/CCordicRotateRom/CCordicRotateRom.cpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/CCordicRotateRom/${CORDIC_ROM_SOURCE} @ONLY
)

add_library (
  cordic_rom_gen OBJECT sources/CCordicRotateRom/${CORDIC_ROM_HEADER}
                        sources/CCordicRotateRom/${CORDIC_ROM_SOURCE} ${ROM_HEADER}
)
target_include_directories (cordic_rom_gen PUBLIC sources)
target_include_directories (cordic_rom_gen SYSTEM PUBLIC ${AP_INCLUDE_DIR})
target_link_libraries (cordic_rom_gen PUBLIC romgen)

file (GLOB ALL_CORDIC_ROM_HEADERS sources/CCordicRotateRom/*.hpp)
file (GLOB ALL_CORDIC_ROM_SOURCES sources/CCordicRotateRom/*.cpp)
file (GLOB ALL_ROM_HEADERS sources/CordicRoms/*.hpp)

add_library (
  cordic STATIC sources/CCordicRotateSmart/CCordicRotateSmart.cpp
                sources/CCordicRotateConstexpr/CCordicRotateConstexpr.cpp
                ${ALL_CORDIC_ROM_SOURCES}
)
target_include_directories (cordic PUBLIC sources)
target_include_directories (cordic SYSTEM PUBLIC ${AP_INCLUDE_DIR})
target_link_libraries (cordic PUBLIC romgen)

# ##################################################################################################

if (ENABLE_TESTING)
  find_package (Catch2 REQUIRED)

  string (CONFIGURE 
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/tb/cordic_rom_tb_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}.cpp
  TB_SOURCE)
  configure_file (
    sources/tb/cordic_rom_tb.cpp.in
    ${TB_SOURCE} @ONLY
  )

  add_library (catch_common OBJECT sources/tb/main_catch2.cpp)
  target_link_libraries (catch_common PUBLIC Catch2::Catch2)

  add_executable (cordic_tb sources/tb/cordic_tb.cpp ${TB_SOURCE})
  target_link_libraries (cordic_tb PUBLIC cordic catch_common)

  include (CTest)
  include (Catch)
  catch_discover_tests (cordic_tb)
endif ()
