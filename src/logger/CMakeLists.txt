add_library(waycast_logger)

target_sources(waycast_logger
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      Logger.cppm
)

find_package(quill CONFIG REQUIRED)

target_link_libraries(waycast_logger
  PUBLIC
    stdcxx
    quill::quill
)
