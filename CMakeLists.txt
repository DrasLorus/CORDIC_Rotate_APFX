cmake_minimum_required (VERSION 3.16.0 FATAL_ERROR)
# setting this is required

set (CMAKE_CXX_STANDARD 14)
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

####################################################################################################
# Options
####################################################################################################

option (EXPORT_COMMANDS "export compile commands, for use with clangd for example." ON)
option (ENABLE_XILINX "use Xilinx provided and proprietary headers." OFF)

option (ENABLE_TESTING "use Catch2 in conjunction with CTest as a test suite." ON)

option (PEDANTIC "use -Wall and -pedantic." ON)

####################################################################################################

if (PEDANTIC)
  add_compile_options(-Wall -pedantic)
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
  find_file (AP_FIXED ap_fixed.h PATH_SUFFIXES ap_types hls_ap_types/include REQUIRED)

  get_filename_component (AP_INCLUDE_DIR ${AP_FIXED} DIRECTORY)
endif ()

if ((NOT EXISTS ${AP_INCLUDE_DIR}/ap_int.h) OR (NOT EXISTS ${AP_INCLUDE_DIR}/ap_fixed.h))
  message (FATAL_ERROR "Arbitrary precision headers not found in ${AP_INCLUDE_DIR}.\n"
                       "Please provide a suitable path to the headers."
  )
endif ()

add_subdirectory (RomGenerators)

add_library (
  cordic STATIC sources/CCordicRotate/CCordicRotate.cpp
                sources/CCordicRotateHalfPiRom/CCordicRotateHalfPiRom.cpp
)
target_include_directories (cordic PUBLIC sources)
target_include_directories (cordic SYSTEM PUBLIC ${AP_INCLUDE_DIR})
target_link_libraries (cordic PUBLIC romgen)

if (ENABLE_TESTING)
  find_package (Catch2 REQUIRED)

  add_library (catch_common OBJECT sources/tb/main_catch2.cpp)
  target_link_libraries (catch_common PUBLIC Catch2::Catch2)

  add_executable (cordic_tb sources/tb/cordic_tb.cpp)
  target_link_libraries (cordic_tb PUBLIC cordic catch_common)

  include (CTest)
  include (Catch)
  catch_discover_tests (cordic_tb)
endif ()
