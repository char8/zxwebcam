find_package(PkgConfig REQUIRED QUIET)

pkg_check_modules(_V4L2 REQUIRED libv4l2)

find_path(V4L2_INCLUDE_DIR
          NAMES libv4l2.h
          HINTS ${_V4L2_INCLUDE_DIRS}
          PATHS /usr/include /usr/local/include)

find_library(V4L2_LIB
             NAMES v4l2
             HINTS ${_V4L2_LIBRARY_DIRS}
             PATHS /usr/lib /usr/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibV4l2 DEFAULT_MSG V4L2_LIB V4L2_INCLUDE_DIR)

if(LIBV4L2_FOUND)
  set(LibV4l2_INCLUDE_DIRS ${V4L2_INCLUDE_DIR})
  set(LibV4l2_LIBRARIES ${V4L2_LIB})
endif()
