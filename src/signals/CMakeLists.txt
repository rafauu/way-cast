add_library(waycast_signals)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

target_sources(waycast_signals
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      Signals.cppm
)

target_link_libraries(waycast_signals
  PUBLIC
    stdcxx
    waycast_logger
)
