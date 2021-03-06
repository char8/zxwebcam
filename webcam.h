#ifndef WEBCAM_H__
#define WEBCAM_H__

#include "frame.h"

#include <spdlog/spdlog.h>
#include <string>
#include <memory>
#include <stdexcept>

namespace zxwebcam {

 
/*! \brief Struct to hold memory mappings for allocated V4L buffers
 * (in kernel memory).
 */
struct BufferMap {
  void* start_; /*!< mapped virt. address, clear after unmap */
  size_t length_;
};

/*! \brief A wrapper around a V4L video input device
 *
 * Wrap a video device and abstract away configuration. Provide
 * and interface that returns ImageMagick Image objects for captured
 * frames.
 */
class Webcam {
  private:
    int fd_; /*!< filedesc of V4L block device */
    bool is_streaming_; /*!< track state of stream */

    std::string device_; /*!< path to device node in /dev/ */
    std::shared_ptr<spdlog::logger> logger_; /*!< ref to spdlogger instance*/

    unsigned int cap_width_; /*!< configured width for captured frames */
    unsigned int cap_height_;  /*!< configured height for captured frames */
    unsigned int fps_; /*!< capture frames per second */
    unsigned int buffer_count_; /*!< num of video buffers allocated by V4L */
    unsigned int pixel_format_; /*!< pixelformat for capture */
    BufferMap* buffers_; /*!< memory mappings of V4L video buffers */

    bool check_capabilities(); /*!< check that the device fulfills min reqs */
    void init_mmap(); /*!< initialise memory mappings */
    void deinit_mmap(); /*!< unmap any active memory mappings */
  public:
    //! Constructor for WebCam objects
    /*!
     *  The constructor will only init variables. Actual device initialisation
     *  happens in \sa init().
     *  
     *  Passed parameters configuring the stream may be changed by the driver
     *  during device init. Warnings should be generated in such a case.
     */
    Webcam(std::string& device, /*!< [in] path to V4L device node */
           unsigned int cap_height = 600, /*!< [in] request height for V4L images.
                                            Value might be overriden by device. */
           unsigned int cap_width = 800, /*!< [in] request width for V4L images.
                                            Value might be overriden by device. */
           unsigned int fps = 5, /*!< [in] request fps for capture. */
           unsigned int buffer_count = 5/*! [in] num of capture buffers to request. */
        );

    //! Will deinit V4L if the device is still open
    ~Webcam(); 

    //! initialises the V4L device
    /*!
     *  This function will perform the following steps using the V4L2 API via
     *  libv4l2.
     *  
     *  1. Open the device
     *  2. Call \sa check_capabilities()
     *  3. Set the stream parameters by calling the VIDIOC_S_FMT ioctl
     *  4. Modify the capture resolution based on the driver response
     *  5. Set the framerate via the VIDIOC_S_PARM ioctl
     *  6. call \sa init_mmap() to configure buffers and memory mapping
     *  
     *  Throws a configuration_error exception on a failure
     */
    void init();

    void close();

    void start_capture();
    void end_capture();
    std::shared_ptr<Frame> grab_frame();

    int fd() const;
    unsigned int cap_width() const;
    unsigned int cap_height() const;
};

}
#endif
