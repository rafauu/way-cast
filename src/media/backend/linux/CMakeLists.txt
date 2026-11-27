add_library(waycast_media_backend)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(LIBPORTAL REQUIRED IMPORTED_TARGET libportal)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

target_sources(waycast_media_backend
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      Xdp.cppm
  PRIVATE
    Xdp.cpp
)

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

target_link_libraries(waycast_media_backend
  PRIVATE
    PkgConfig::GSTREAMER
    PkgConfig::LIBPORTAL
    waycast_logger
    waycast_media_core
)
