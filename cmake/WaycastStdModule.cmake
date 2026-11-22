# -----------------------------------------------------------------------------
# GCC 15 std module workaround
# -----------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16
)
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" -E -x c++ - -v
    INPUT_FILE /dev/null
    ERROR_VARIABLE GCC_INCLUDE_OUTPUT
    OUTPUT_QUIET
    RESULT_VARIABLE GCC_INCLUDE_RESULT
  )

  if(NOT GCC_INCLUDE_RESULT EQUAL 0)
    message(FATAL_ERROR
      "Failed to query GCC include directories:\n${GCC_INCLUDE_OUTPUT}"
    )
  endif()

  string(REGEX MATCHALL
    "[^\n\r]+"
    GCC_INCLUDE_LINES
    "${GCC_INCLUDE_OUTPUT}"
  )

  set(GCC_CXX_INCLUDE_DIRS)

  foreach(LINE IN LISTS GCC_INCLUDE_LINES)
    string(STRIP "${LINE}" LINE)

    if(IS_DIRECTORY "${LINE}")
      list(APPEND GCC_CXX_INCLUDE_DIRS "${LINE}")
    endif()
  endforeach()

  find_file(
    GCC_STD_MODULE
    std.cc
    PATHS ${GCC_CXX_INCLUDE_DIRS}
    PATH_SUFFIXES bits
    NO_DEFAULT_PATH
  )

  if(NOT GCC_STD_MODULE)
    message(FATAL_ERROR
      "Could not find std.cc for GCC "
      "${CMAKE_CXX_COMPILER_VERSION}.\n"
      "Searched:\n${GCC_CXX_INCLUDE_DIRS}"
    )
  endif()

  get_filename_component(
    GCC_STD_DIR
    "${GCC_STD_MODULE}"
    DIRECTORY
  )

  add_library(stdcxx OBJECT)

  target_sources(stdcxx PUBLIC
      FILE_SET std_module TYPE CXX_MODULES BASE_DIRS "${GCC_STD_DIR}" FILES ${GCC_STD_MODULE}
  )

  link_libraries(stdcxx)

  message(STATUS "")
  message(STATUS "============================================================")
  message(STATUS "GCC Modules")
  message(STATUS "============================================================")
  message(STATUS "GCC std module: ${GCC_STD_MODULE}")
  message(STATUS "GCC std directory: ${GCC_STD_DIR}")
  message(STATUS "============================================================")
endif()

# -----------------------------------------------------------------------------
# GCC 16 std module
# -----------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
    set(CMAKE_CXX_MODULE_STD ON)
endif()
