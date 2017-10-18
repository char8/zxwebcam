#include "webcam.h"

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
namespace mgk = Magick;

/* Taken from v4lgrab.c example
 * https://chromium.googlesource.com/chromiumos/third_party/kernel/+/master/Documentation/video4linux/v4lgrab.c
 * removed loop on EINTR as I don't think we need it here
 */
static int xioctl(int fd, unsigned long request, void *arg) {
  int res;

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
  is_open_{false},
  device_{device},
  logger_{spd::get("v4l")},
  cap_width_{cap_height},
  cap_height_{cap_width},
  fps_{fps},
  buffer_count_{buffer_count},
  buffers_{nullptr} {
}

Webcam::~Webcam() {
  //TODO: stop stream
  //TODO: unmap memory
  close();
}

void Webcam::close() {
  if (is_open_) {
    v4l2_close(fd_);
    fd_ = -1;
  }
}

void Webcam::init() {
  logger_->debug("Opening V4L device: " + device_);
  fd_ = v4l2_open(device_.c_str(), O_RDWR | O_NONBLOCK, 0);
  
  if (fd_ == -1) {
    throw configuration_error("Unable to open V4L device with err={}",
                              errno);
  }

  is_open_ = true;

  if (!check_capabilities()) 
    // Device doesn't support Video capture or streaming
    throw configuration_error("Device is not supported.");

  v4l2_format vfmt          = {};
  vfmt.type                 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vfmt.fmt.pix.width        = cap_width_;
  vfmt.fmt.pix.height       = cap_height_;
  vfmt.fmt.pix.pixelformat  = V4L2_PIX_FMT_MJPEG;
  vfmt.fmt.pix.field        = V4L2_FIELD_NONE;

  if (-1 == xioctl(fd_, VIDIOC_S_FMT, &vfmt))
    throw configuration_error("Unable to set device format.", errno);

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
    throw configuration_error("Unable to set framerate.", errno);

  init_mmap();
}

void Webcam::init_mmap() {
  v4l2_requestbuffers reqbuffers = {};
  
  reqbuffers.count = buffer_count_;
  reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuffers.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd_, VIDIOC_REQBUFS, &reqbuffers))
    throw configuration_error("Unable to configure buffers. Errno: {}", errno);
  

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
      throw configuration_error("Buffer configuration failed", errno);
    }

    buffers_[n].length_ = buf.length;
    buffers_[n].start_ = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_, buf.m.offset);
    
    if (MAP_FAILED == buffers_[n].start_) {
      throw configuration_error("Memory mapping failed", errno);
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
      throw configuration_error("Buffer configuration failed", errno);
    }
  }

  // enable streaming
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl(fd_, VIDIOC_STREAMON, &type)) {
    throw configuration_error("Unable to start streaming", errno);
  }
}

void Webcam::end_capture() {
  v4l2_buf_type type;

  if (-1 == xioctl(fd_, VIDIOC_STREAMOFF, &type)) {
    throw configuration_error("Unable to stop streaming", errno); 
  }
}

int Webcam::grab_frame(mgk::Blob& dest_blob) {
  // Refactor?: return a SharedPtr to a CamFrame obj that is composed
  // of a blob and timestamp. Nullptr return on EAGAIN.
  v4l2_buffer buf = {};
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd_, VIDIOC_DQBUF, &buf)) {
     switch(errno) {
      case EAGAIN:
        return 0; // Nonblocking read
      default:
        logger_->error("Unable to dequeue buffer from device. Errno: {}.", errno);
        exit(EXIT_FAILURE);
     }
  }

  dest_blob.update(buffers_[buf.index].start_, buffers_[buf.index].length_);

  // enqueue the frame again
  if (-1 == xioctl(fd_, VIDIOC_QBUF, &buf)) {
    logger_->error("Unable to requeue frame {}. Errno: {}.", buf.index, errno);
    exit(EXIT_FAILURE);
  }
  return 1;
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
