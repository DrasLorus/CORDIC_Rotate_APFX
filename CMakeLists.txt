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

cmake_minimum_required (VERSION 3.18.0 FATAL_ERROR)
# setting this is required

project (
  CordicRotate
  LANGUAGES CXX
  VERSION 0.1
)

if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.2))
  set (CMAKE_CXX_STANDARD 11)
else ()
  set (CMAKE_CXX_STANDARD 14)
endif ()
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set (CMAKE_EXPORT_COMPILE_COMMANDS true)

# ##################################################################################################
# Options
# ##################################################################################################

option (EXPORT_COMMANDS "export compile commands, for use with clangd for example." ON)
option (ENABLE_XILINX "use Xilinx provided proprietary headers." OFF)
option (ENABLE_TESTING "use Catch2 (if possible) in conjunction with CTest as a test suite." ON)
option (ENABLE_SOFTWARE
        "use C++ standard library types (like std::complex). Unsuitable for synthesis." ON
)

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
    "2019.1"
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

if (ENABLE_SOFTWARE)
  add_compile_definitions (SOFTWARE=1)
endif ()

set (
  ROM_TYPE
  "ml"
  CACHE STRING "RomGenerator to use, either 'ml' or 'cst'."
)
set_property (CACHE ROM_TYPE PROPERTY STRINGS "ml" "cst")

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
  CACHE STRING "rotation denominator."
)

add_subdirectory (RomGenerators)

set (ROM_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sources/CordicRoms)
string (
  CONFIGURE
    ${ROM_DIRECTORY}/cordic_rom_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}_${CORDIC_DIVIDER}.hpp
    ROM_HEADER
)
add_custom_command (
  OUTPUT ${ROM_HEADER}
  COMMAND rom_generator
  WORKING_DIRECTORY ${ROM_DIRECTORY}
)

set (CORDIC_ROM_HEADER
     CCordicRotateRom_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}_${CORDIC_DIVIDER}.hpp
)
set (CORDIC_ROM_SOURCE
     CCordicRotateRom_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}_${CORDIC_DIVIDER}.cpp
)
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

file (GLOB ALL_CORDIC_ROM_SOURCES sources/CCordicRotateRom/*.cpp)
list (REMOVE_ITEM ALL_CORDIC_ROM_SOURCES sources/CCordicRotateRom/${CORDIC_ROM_SOURCE})

add_library (cordic STATIC ${ALL_CORDIC_ROM_SOURCES})
if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 6.2
                                               )
)
  target_sources (
    cordic PRIVATE sources/CCordicRotateSmart/CCordicRotateSmart.cpp
                   sources/CCordicRotateConstexpr/CCordicRotateConstexpr.cpp
  )
endif ()
target_include_directories (cordic PUBLIC sources)
target_include_directories (cordic SYSTEM PUBLIC ${AP_INCLUDE_DIR})
target_link_libraries (cordic PUBLIC romgen cordic_rom_gen)

# ##################################################################################################

if (ENABLE_TESTING)
  include (CTest)
  if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.2))
    string (
      CONFIGURE
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/tb/catch_less/cordic_rom_aptypes_tb_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}_${CORDIC_DIVIDER}.cpp
        TB_SOURCE
    )
    configure_file (sources/tb/catch_less/cordic_rom_aptypes_tb.cpp.in ${TB_SOURCE} @ONLY)

    file (GLOB ALL_ROM_TB_SOURCES sources/tb/catch_less/cordic_rom_aptypes*.cpp)
    list (REMOVE_ITEM ALL_ROM_TB_SOURCES ${TB_SOURCE})

    get_filename_component (FILE_RADIX ${TB_SOURCE} NAME_WE)
    add_executable (${FILE_RADIX} ${TB_SOURCE})
    target_link_libraries (${FILE_RADIX} PRIVATE cordic)

    add_test (
      NAME ${FILE_RADIX}
      COMMAND ${FILE_RADIX}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data
    )

    foreach (TB_FILE ${ALL_ROM_TB_SOURCES})
      get_filename_component (FILE_RADIX ${TB_FILE} NAME_WE)
      add_executable (${FILE_RADIX} ${TB_FILE})
      target_link_libraries (${FILE_RADIX} PRIVATE cordic)

      add_test (
        NAME ${FILE_RADIX}
        COMMAND ${FILE_RADIX}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data
      )
    endforeach ()

  else ()
    find_package (Catch2 REQUIRED)

    string (
      CONFIGURE
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/tb/catchy/cordic_rom_tb_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}_${CORDIC_DIVIDER}.cpp
        TB_SOURCE
    )

    configure_file (sources/tb/catchy/cordic_rom_tb.cpp.in ${TB_SOURCE} @ONLY)

    add_library (catch_common_${PROJECT_NAME} OBJECT sources/tb/catchy/main_catch2.cpp)
    target_link_libraries (catch_common_${PROJECT_NAME} PUBLIC Catch2::Catch2)

    file (GLOB ALL_ROM_TB_SOURCES sources/tb/catchy/cordic_rom_*.cpp)
    list (REMOVE_ITEM ALL_ROM_TB_SOURCES ${TB_SOURCE})

    add_executable (cordic_tb sources/tb/catchy/cordic_tb.cpp ${TB_SOURCE} ${ALL_ROM_TB_SOURCES})
    target_link_libraries (cordic_tb PRIVATE cordic catch_common_${PROJECT_NAME})

    include (Catch)
    catch_discover_tests (cordic_tb WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data)
  endif ()
endif ()

file (GLOB ALL_ROM_HEADERS sources/CordicRoms/cordic_rom_*.hpp)
file (GLOB ALL_CORDIC_ROM_HEADERS sources/CCordicRotateRom/CCordicRotateRom_*.hpp)
add_custom_target (
  remove_byproducts
  COMMAND ${CMAKE_COMMAND} -E echo "-- Deleting generated sources files and headers"
  COMMAND ${CMAKE_COMMAND} -E rm -f ${TB_SOURCE} ${ALL_ROM_TB_SOURCES}
  COMMAND ${CMAKE_COMMAND} -E rm -f ${CORDIC_ROM_SOURCE} ${ALL_CORDIC_ROM_SOURCES}
  COMMAND ${CMAKE_COMMAND} -E rm -f ${ALL_ROM_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E rm -f ${ALL_CORDIC_ROM_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E echo "-- Files deleted."
  COMMAND ${CMAKE_COMMAND} -E echo
          "-- WARNING: You must re-run the cmake-configure step for the next build to succeed."
)
