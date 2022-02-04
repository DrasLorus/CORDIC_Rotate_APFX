cmake_minimum_required (VERSION 3.16.0 FATAL_ERROR)
# setting this is required

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set (CMAKE_EXPORT_COMPILE_COMMANDS true)

project (
  CordicRotate
  LANGUAGES CXX
  VERSION 0.1
)

option (EXPORT_COMMANDS "Export compile commands, for use with clangd for example." ON)
if (EXPORT_COMMANDS)
    set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()


option (ENABLE_XILINX "use Xilinx provided and proprietary headers." ON)
if (ENABLE_XILINX)
  set (
    XILINX_HOME
    /opt/Xilinx
    CACHE PATH "path to Xilinx root folder."
  )
  set (
    XILINX_VER
    "2020.2"
    CACHE STRING "Xilinx software version to use." FORCE
  )

  if (XILINX_VER VERSION_GREATER_EQUAL "2020.1")
    set (AP_INCLUDE_DIR ${XILINX_HOME}/Vitis_HLS/${XILINX_VER}/include)
  else ()
    set (AP_INCLUDE_DIR ${XILINX_HOME}/Vivado/${XILINX_VER}/include)
  endif ()
  message (STATUS "AP headers must lie under ${AP_INCLUDE_DIR}")
else ()
  set (
    HLS_AP_T_DIR
    ${CMAKE_SOURCE_DIR}/../../Utilities/C++/hls_ap_types
    CACHE PATH "path to a clone of HLS_arbitrary_Precision_Types."
  )
  set (AP_INCLUDE_DIR ${HLS_AP_T_DIR}/include)
endif ()

if ((NOT EXISTS ${AP_INCLUDE_DIR}/ap_int.h) OR (NOT EXISTS
                                                ${AP_INCLUDE_DIR}/ap_fixed.h)
)
  message (
    FATAL_ERROR
      "Arbitrary precision headers not found in ${AP_INCLUDE_DIR}.\n"
      "Consider disabling the ap_int feature using `-DENABLE_AP_INT=OFF`"
      " or provide a suitable path to the headers."
  )
endif ()

add_library (cordic STATIC sources/CCordicRotate/CCordicRotate.cpp)
target_include_directories (cordic SYSTEM PUBLIC ${AP_INCLUDE_DIR})

