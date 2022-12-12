cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(get-pspdev-path)

# Note this one is already set in the build script.
# We set it again here in case the build script isn't used.
set(CMAKE_INSTALL_PREFIX "${PSPDEV}" CACHE STRING "Default install prefix")

set(PSPSDK_TOOL_PREFIX_LIB "" CACHE STRING "CMake target name prefix for the internal libs (libs)")
set(PSPSDK_TOOL_PREFIX_TOOL "" CACHE STRING "CMake target name prefix for the pspsdk tools (tools)")
set(PSPSDK_TOOL_NAMESPACE "pspsdk::" CACHE STRING "CMake target namespace for the exported pspsdk tools")
set(PSPSDK_TOOL_EXPORT_NAME "pspsdk-tools-targets" CACHE STRING "CMake targets export file name")

project(
  pspsdk_tools
  VERSION 1.0 LANGUAGES C
  DESCRIPTION "PSPSDK build and utility tools")

add_subdirectory(libs)
add_subdirectory(tools)
