CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(zxwebcam-cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(CMakeLists-ExtZxing.txt)
include_directories(${ZXING_INCLUDE_DIR} ${SPDLOG_INCLUDE_DIR})

FIND_PACKAGE(LibV4l2)
include_directories(${LibV4l2_INCLUDE_DIRS})

FIND_PACKAGE(CImg)
include_directories(${CIMG_INCLUDE_DIRS})

FIND_PACKAGE(Threads)

SET (SRCS decode.cxx webcam.cxx webcam_thread.cxx frame_source.cxx decode.cxx)
SET (LIBS ${LIBS} zxing ${LibV4l2_LIBRARIES} ${CIMG_EXT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -ggdb -DSPDLOG_DEBUG_ON")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CIMG_CFLAGS}")

find_package(X11 REQUIRED)

include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})

add_executable(zxwebcam main.cxx ${SRCS})
target_link_libraries(zxwebcam ${LIBS} ${X11_LIBRARIES})
add_dependencies(zxwebcam spdlog zxing-cpp)

