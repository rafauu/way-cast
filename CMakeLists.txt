cmake_minimum_required(VERSION 4.2)

project(waycast LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Build configuration
# -----------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# mold has a SIOF bug in interop with glib
#if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
#  set(CMAKE_LINKER_TYPE MOLD)
#endif()

# -----------------------------------------------------------------------------
# CMake modules
# -----------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(WaycastOptions)
include(WaycastStdModule)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

set(WAYCAST_VCPKG_PREFIX
  "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}"
)

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

add_subdirectory(src)

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "WayCast")
message(STATUS "============================================================")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:         ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "vcpkg triplet:    ${VCPKG_TARGET_TRIPLET}")
message(STATUS "vcpkg prefix:     ${WAYCAST_VCPKG_PREFIX}")
message(STATUS "============================================================")
