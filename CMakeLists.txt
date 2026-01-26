cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_MODULE_STD OFF)

project(HlsCast CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(GST_BASE REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
#pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
find_package(Boost CONFIG REQUIRED COMPONENTS coroutine system)
find_package(readerwriterqueue CONFIG REQUIRED)



add_library(app_modules)

target_sources(app_modules PUBLIC
    FILE_SET CXX_MODULES FILES
        src/compat/std_module.cppm # bug in cmake3/gcc15
        src/core_queue.cppm
        src/hls_engine.cppm
        src/media_streamer.cppm
        src/media_processor.cppm
        src/net_server.cppm
    PRIVATE
        src/net_server_impl.cpp # lack of support for module impl
)

#target_compile_options(app_modules PRIVATE -fmodules-ts -fno-module-lazy)
target_compile_options(app_modules PRIVATE -fmodules-ts)

target_link_libraries(app_modules PRIVATE
    PkgConfig::GSTREAMER
    PkgConfig::GST_APP
    PkgConfig::GST_BASE
    #PkgConfig::GIO
    #${GIO_LIBRARIES}
    readerwriterqueue::readerwriterqueue
)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE app_modules)
target_link_options(app PRIVATE "-Wl,--export-dynamic")

target_include_directories(app SYSTEM PRIVATE ${GIO_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${GIO_LIBRARIES})
#target_compile_options(your_app_name PRIVATE ${GIO_CFLAGS_OTHER})
