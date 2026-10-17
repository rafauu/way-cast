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

set(CMAKE_LINKER_TYPE MOLD)

set(WAYCAST_VCPKG_PREFIX
  "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}"
)

# -----------------------------------------------------------------------------
# GCC std module
# -----------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(REGEX MATCH "^[0-9]+" GCC_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")

  foreach(PREFIX /usr /usr/local)
    if(EXISTS "${PREFIX}/include/c++/${GCC_MAJOR}/bits/std.cc")
      set(GCC_STD_MODULE "${PREFIX}/include/c++/${GCC_MAJOR}/bits/std.cc")
      set(GCC_STD_DIR "${PREFIX}/include/c++/${GCC_MAJOR}/bits")
      break()
    endif()
  endforeach()

  if(NOT GCC_STD_MODULE)
    message(FATAL_ERROR "Could not find std.cc for GCC ${GCC_MAJOR}.")
  endif()
endif()

# -----------------------------------------------------------------------------
# Subdirectories
# -----------------------------------------------------------------------------

add_subdirectory(src)

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "WayCast")
message(STATUS "============================================================")
message(STATUS "Build type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:            ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version:    ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "vcpkg triplet:       ${VCPKG_TARGET_TRIPLET}")
message(STATUS "vcpkg prefix:        ${WAYCAST_VCPKG_PREFIX}")
message(STATUS "============================================================")
