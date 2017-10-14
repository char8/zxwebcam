#include <sys/ioctl.h>
#include <error.h>
#include <fcntl.h>
#include <iostream>
#include <spdlog/spdlog.h>

#include <linux/videodev2.h>
#include <libv4l2.h>

namespace spd = spdlog;

class V4l2Wrapper {
  private:
    int fd_;
    bool isOpen_; // track v4l device state

    std::string device_;
    std::shared_ptr<spd::logger> logger_;

    unsigned int imgWidth_;
    unsigned int imgHeight_;
    unsigned int fps_;
  public:
    V4l2Wrapper(std::string& device);
    ~V4l2Wrapper(); 
    void init();
    bool checkCapabilities();
    void initMmap();

    void close();
    void queue();
    void grab();
};


V4l2Wrapper::V4l2Wrapper(std::string& device): fd_{-1}, isOpen_{false}, device_{device},
  logger_{spd::get("v4l")}, imgWidth_{800}, imgHeight_{600}, fps_{5} {
}

V4l2Wrapper::~V4l2Wrapper() {
  close();
}

void V4l2Wrapper::close() {
  if (isOpen_) {
    v4l2_close(fd_);
    fd_ = -1;
  }
}

static int xioctl(int fd, int request, void *arg) {
  int res;

  do {
    res = v4l2_ioctl(fd, request, arg);
  } while ((-1 == res) && (EAGAIN == errno));

  return res;
}

void V4l2Wrapper::init() {
  logger_->debug("Opening V4L device: " + device_);
  fd_ = v4l2_open(device_.c_str(), O_RDWR | O_NONBLOCK, 0);
  
  int en = errno;
  if (fd_ == -1) {
    logger_->error("Unable to open V4L device with err={}", en);
    exit(EXIT_FAILURE);
  }

  isOpen_ = true;

  if (!checkCapabilities()) {
    // Device doesn't support Video capture or streaming
    logger_->error("Device is not supported.");
    exit(EXIT_FAILURE);
  }

  v4l2_format vfmt = {0};
  
  vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vfmt.fmt.pix.width = imgWidth_;
  vfmt.fmt.pix.height = imgHeight_;
  vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
  vfmt.fmt.pix.field = V4L2_FIELD_NONE;

  if (-1 == xioctl(fd_, VIDIOC_S_FMT, &vfmt)) {
    logger_->error("Unable to set device format.");
    exit(EXIT_FAILURE);
  }

  if ((imgWidth_ != vfmt.fmt.pix.width) ||
      (imgHeight_ != vfmt.fmt.pix.height)) {
    imgWidth_ = vfmt.fmt.pix.width;
    imgHeight_ = vfmt.fmt.pix.height;
    logger_->warn("Device changed image dimensions to {}x{}",
        imgWidth_,
        imgHeight_);
  }

  v4l2_streamparm sparm = {0};
  sparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  sparm.parm.capture.timeperframe = {1, fps_};
  
  if (-1 == xioctl(fd_, VIDIOC_S_PARM, &sparm)) {
    logger_->error("Unable to set framerate.");
    exit(EXIT_FAILURE);
  }

  initMmap();
}

void V4l2Wrapper::initMmap() {


}

bool V4l2Wrapper::checkCapabilities() {
  v4l2_capability cap = {0};

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

  v4l2_format fmt = {0};
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

int main(void) {
  auto console = spdlog::stdout_color_mt("console");
  console->info("Welcome to stdlog!");
  auto v4l = spdlog::stdout_color_mt("v4l");
  std::string d = "/dev/video0";
  V4l2Wrapper v(d);
  v.init();
  v.close();
  return 0;
}
