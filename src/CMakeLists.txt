add_library(stdcxx)
target_sources(stdcxx
  PUBLIC
    FILE_SET std_module TYPE CXX_MODULES BASE_DIRS "${GCC_STD_DIR}" FILES ${GCC_STD_MODULE}
)

add_subdirectory(env)
add_subdirectory(logger)
add_subdirectory(media)

add_executable(${PROJECT_NAME}
  main.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
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
    stdcxx
    waycast_env
    waycast_logger
    waycast_media
)
