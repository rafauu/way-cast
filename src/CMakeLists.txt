# -----------------------------------------------------------------------------
# Components
# -----------------------------------------------------------------------------

add_subdirectory(env)
add_subdirectory(logger)
add_subdirectory(media)
add_subdirectory(signals)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------

add_executable(${PROJECT_NAME}
  main.cpp
)

if(WIN32)
  target_sources(${PROJECT_NAME} PRIVATE waycast.rc)
endif()

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

set_target_properties(${PROJECT_NAME} PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    waycast_options
    waycast_env
    waycast_logger
    waycast_media
    waycast_signals
)
