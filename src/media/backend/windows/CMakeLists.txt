add_library(waycast_media_backend)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

target_sources(waycast_media_backend
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      Win32.cppm
  PRIVATE
    Win32.cpp
    Win32Impl.cpp
)

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

target_link_libraries(waycast_media_backend
  PRIVATE
    runtimeobject
    comctl32
    waycast_logger
)
