CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(zxwebcam-cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

FIND_PACKAGE(LibV4l2 REQUIRED)
include_directories(${LibV4l2_INCLUDE_DIRS})

FIND_PACKAGE(CImg REQUIRED)
include_directories(${CIMG_INCLUDE_DIRS})

FIND_PACKAGE(Threads REQUIRED)

add_subdirectory(3rdparty)

SET (SRCS decode_thread.cxx
          webcam.cxx
          webcam_thread.cxx
          reader.cxx)

SET (LIBS ${LIBS}
          ZXingCore
          ${LibV4l2_LIBRARIES}
          ${CIMG_EXT_LIBRARIES}
          ${CMAKE_THREAD_LIBS_INIT}
          spdlog)

# detect X11
find_package(X11)

if (X11_FOUND)
  message(STATUS "Compiling with X11 support")
  include_directories(${X11_INCLUDE_DIR})
  link_directories(${X11_LIBRARIES})
  add_definitions(-Dcimg_display=1)
  add_definitions(-DXDISPLAY)
else (X11_FOUND)
  message(STATUS "compiling without X11 support")
  add_definitions(-Dcimg_display=0)
endif (X11_FOUND)

# set CFLAGS
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -ggdb")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CIMG_CFLAGS}")


add_executable(zxwebcam main.cxx ${SRCS})
target_link_libraries(zxwebcam ${LIBS} ${X11_LIBRARIES})

