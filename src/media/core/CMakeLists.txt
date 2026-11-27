add_library(waycast_media_core)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  pkg_check_modules(LIBPORTAL REQUIRED IMPORTED_TARGET libportal)
endif()

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

target_sources(waycast_media_core
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      Raii.cppm
)

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

target_link_libraries(waycast_media_core PRIVATE PkgConfig::GSTREAMER)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(waycast_media_core PRIVATE PkgConfig::LIBPORTAL)
endif()
