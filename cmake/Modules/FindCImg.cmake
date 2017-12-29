# based off
# https://github.com/charmie11/CImg-hello-world/blob/master/extern/FindCImg.cmake

# Vars provided
# CIMG_FOUND
# CIMG_INCLUDE_DIRS
# CIMG_EXT_LIBRARIES
# CIMG_EXT_LIBRARIES_DIRS
# CIMG_CFLAGS

if (CIMG_INCLUDE_DIR)
  set(CIMG_FOUND TRUE)
else (CIMG_INCLUDE_DIR)
  find_path(CIMG_INCLUDE_DIR
          NAMES CImg.h
          PATHS ${CMAKE_SOURCE_DIR}/3rdparty/CImg /usr/include)
endif(CIMG_INCLUDE_DIR)

list(APPEND CIMG_INCLUDE_DIRS
     ${CIMG_INCLUDE_DIR})

set(CIMG_CFLAGS "-Dcimg_display=0")

find_package(JPEG)

if(JPEG_FOUND)
  get_filename_component(JPEG_LIB_DIR ${JPEG_LIBRARIES} PATH)
  set(CIMG_CFLAGS "${CImg_CFLAGS} -Dcimg_use_jpeg")
  list(APPEND CIMG_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})
  list(APPEND CIMG_EXT_LIBRARIES ${JPEG_LIBRARIES})
  list(APPEND CIMG_EXT_LIBRARIES_DIRS ${JPEG_LIB_DIR})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CIMG DEFAULT_MSG
                                  CIMG_INCLUDE_DIR)


