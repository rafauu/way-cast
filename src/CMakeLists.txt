add_subdirectory(env)
add_subdirectory(logger)
add_subdirectory(media)
add_subdirectory(signals)

add_executable(${PROJECT_NAME}
  main.cpp
)

if(WIN32)
  target_sources(${PROJECT_NAME} PRIVATE waycast.rc)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
    -Wall
    -Wextra
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-interference-size>
    $<$<CONFIG:Release>:-march=native>
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    waycast_env
    waycast_logger
    waycast_media
    waycast_signals
)
