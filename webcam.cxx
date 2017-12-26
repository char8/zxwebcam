#include "webcam.h"

#include <system_error>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <error.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <libv4l2.h>

//using namespace zxwebcam::Webcam;
//using namespace zxwebcam::configuration_error;
using namespace zxwebcam;

namespace spd = spdlog;

/* Taken from v4lgrab.c example
 * https://chromium.googlesource.com/chromiumos/third_party/kernel/+/master/Documentation/video4linux/v4lgrab.c
 * removed loop on EINTR as I don't think we need it here
 */
static int xioctl(int fd, unsigned long request, void *arg) {
  int res = -1;

  do {
    res = v4l2_ioctl(fd, request, arg);
  } while ((-1 == res) && (EAGAIN == errno));

  return res;
}

Webcam::Webcam(std::string& device,
    unsigned int cap_height,
    unsigned int cap_width,
    unsigned int fps,
    unsigned int buffer_count):
  fd_{-1},
  is_streaming_{false},
  device_{device},
  logger_{spd::get("console")},
  cap_width_{cap_width},
  cap_height_{cap_height},
  fps_{fps},
  buffer_count_{buffer_count},
  buffers_{nullptr} {
}

Webcam::~Webcam() {
  // destructor can throw exceptions which is not ideal
  // close shouldn't have anything to do when this is called...
  close();
}

void Webcam::close() {
  if (fd_ != -1) {

    if (is_streaming_)
      end_capture();

    for (unsigned int n = 0; n < buffer_count_; n++) {
      int r = munmap(buffers_[n].start_, buffers_[n].length_);
      if (-1 == r) {
        throw std::system_error(errno, std::generic_category(), "Unable to unmap buffers from V4L2");
      }

      buffers_[n].start_ = nullptr;
      buffers_[n].length_ = 0;
    }

    delete[] buffers_;
    buffers_ = nullptr;

    v4l2_close(fd_);
    fd_ = -1;
  }
}

void Webcam::init() {
  logger_->debug("Opening V4L device: " + device_);
  
  fd_ = v4l2_open(device_.c_str(), O_RDWR | O_NONBLOCK, 0);

  if (fd_ == -1) {
    throw std::system_error(errno, std::generic_category(), "Unable to open V4L device with err");
  }

  if (!check_capabilities()) 
    // Device doesn't support Video capture or streaming
    throw std::runtime_error("Device is not supported.");

  v4l2_format vfmt          = {};
  vfmt.type                 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vfmt.fmt.pix.width        = cap_width_;
  vfmt.fmt.pix.height       = cap_height_;
  vfmt.fmt.pix.pixelformat  = V4L2_PIX_FMT_MJPEG;
  vfmt.fmt.pix.field        = V4L2_FIELD_NONE;

  if (-1 == xioctl(fd_, VIDIOC_S_FMT, &vfmt))
    throw std::system_error(errno, std::generic_category(), "Unable to set device format.");

  if ((cap_width_ != vfmt.fmt.pix.width) ||
      (cap_height_ != vfmt.fmt.pix.height)) {
    cap_width_ = vfmt.fmt.pix.width;
    cap_height_ = vfmt.fmt.pix.height;
    logger_->warn("Device changed image dimensions to {}x{}",
        cap_width_,
        cap_height_);
  }

  v4l2_streamparm sparm = {};
  sparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  sparm.parm.capture.timeperframe = {1, fps_};
  if (-1 == xioctl(fd_, VIDIOC_S_PARM, &sparm))
    throw std::system_error(errno, std::generic_category(), "Unable to set framerate.");

  init_mmap();

  logger_->debug("Initialised {}", device_);
}

void Webcam::init_mmap() {

  if (buffers_ != nullptr)
    throw std::runtime_error("Should not re-init mmap while one exists");

  v4l2_requestbuffers reqbuffers = {};
  
  reqbuffers.count = buffer_count_;
  reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuffers.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd_, VIDIOC_REQBUFS, &reqbuffers))
    throw std::system_error(errno, std::generic_category(), "Unable to configure buffers. Errno: {}");
  

  if (reqbuffers.count != buffer_count_)
    logger_->warn("Driver was only able to allocate {} buffers.", reqbuffers.count);


  buffer_count_ = reqbuffers.count;
  buffers_ = new BufferMap[buffer_count_]; //allocate memory for buffer mapping information
  
  // clear buf maps array in case we need to unmap during the following loop
  // because of failure
  for (unsigned int n = 0; n < buffer_count_; n++) {
    buffers_[n].start_ = nullptr;
    buffers_[n].length_ = 0;
  }

  for (unsigned int n = 0; n < buffer_count_; n++) {
    v4l2_buffer buf = {};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n;

    if (-1 == xioctl(fd_, VIDIOC_QUERYBUF, &buf)) {
      logger_->error("Querying buffer {} failed.", n);
      throw std::system_error(errno, std::generic_category(), "Buffer configuration failed");
    }

    buffers_[n].length_ = buf.length;
    buffers_[n].start_ = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_, buf.m.offset);
    
    if (MAP_FAILED == buffers_[n].start_) {
      throw std::system_error(errno, std::generic_category(), "Memory mapping failed");
    }
  }
}

void Webcam::start_capture() {
  v4l2_buf_type type;

  for (unsigned int n = 0; n < buffer_count_; n++) {
    // Queue buffers
    v4l2_buffer buf = {};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n;

    if (-1 == xioctl(fd_, VIDIOC_QBUF, &buf)) {
      logger_->error("Unable to queue buffer {}. Errno: {}.", n, errno);
      throw std::system_error(errno, std::generic_category(), "Buffer configuration failed");
    }
  }

  // enable streaming
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl(fd_, VIDIOC_STREAMON, &type)) {
    throw std::system_error(errno, std::generic_category(), "Unable to start streaming");
  }

  is_streaming_ = true;
  
  logger_->debug("Starting capture on {}", device_);
}

void Webcam::end_capture() {
  v4l2_buf_type type;

  if (-1 == xioctl(fd_, VIDIOC_STREAMOFF, &type)) {
    throw std::system_error(errno, std::generic_category(), "Unable to stop streaming"); 
  }

  is_streaming_ = false;

  logger_->debug("Ending capture on {}", device_);
}

std::shared_ptr<Frame> Webcam::grab_frame() {
  // Refactor?: return a SharedPtr to a CamFrame obj that is composed
  // of a blob and timestamp. Nullptr return on EAGAIN.
  v4l2_buffer buf = {};
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd_, VIDIOC_DQBUF, &buf)) {
     switch(errno) {
      case EAGAIN:
        return nullptr; // Nonblocking read
      default:
        throw std::system_error(errno,
                std::system_category(),
                "Unable to dequeue buffer from device.");
     }
  }
  
  auto f = std::make_shared<Frame>((char*)buffers_[buf.index].start_,
                                   FrameEncoding::JPEG,
                                   buf.bytesused);
    
  // enqueue the frame again
  if (-1 == xioctl(fd_, VIDIOC_QBUF, &buf)) {
    throw std::system_error(errno,
             std::system_category(),
             "Unable to requeue frame.");
  }
  return f;
}

int Webcam::fd() const {
  return fd_;
}

unsigned int Webcam::cap_height() const {
  return cap_height_;
}

unsigned int Webcam::cap_width() const {
  return cap_width_;
}

bool Webcam::check_capabilities() {
  v4l2_capability cap = {};

  if (-1 == xioctl(fd_, VIDIOC_QUERYCAP, &cap)) {
    logger_->error("Unable to query v4l device.");
    return false;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    logger_->error("Device does not support video capture.");
    return false;
  }

  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    logger_->error("Device does not support streaming.");
    return false;
  }

  v4l2_format fmt = {};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl(fd_, VIDIOC_G_FMT, &fmt)) {
    logger_->error("Could not query supported formats.");
    return false;
  }

  if (!(fmt.fmt.pix.pixelformat & V4L2_PIX_FMT_MJPEG)) {
    logger_->error("Does not support MJPEG output.");
    return false;
  }

  return true;
}
