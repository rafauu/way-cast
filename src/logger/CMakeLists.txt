add_library(waycast_logger)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(quill CONFIG REQUIRED)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

target_sources(waycast_logger
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      Logger.cppm
  PRIVATE
    Logger.cpp
)

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

target_link_libraries(waycast_logger
  PRIVATE
    quill::quill
)
