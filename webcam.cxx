#include "webcam.h"

#include <sys/select.h>
#include <sys/time.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <error.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <fstream>

#include "MagickSource.h"
#include <vector>

#include <zxing/Binarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/ReaderException.h>
#include <zxing/Result.h>
#include <zxing/Exception.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>
#include <zxing/MultiFormatReader.h>

namespace spd = spdlog;
namespace mgk = Magick;

Webcam::Webcam(std::string& device): fd_{-1}, isOpen_{false}, device_{device},
  logger_{spd::get("v4l")}, imgWidth_{800}, imgHeight_{600}, fps_{5},
  buffer_count_{5}, buffers_{nullptr} {
}

Webcam::~Webcam() {
  close();
}

void Webcam::close() {
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

void Webcam::init() {
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

void Webcam::initMmap() {
  v4l2_requestbuffers reqbuffers = {0};
  
  reqbuffers.count = buffer_count_;
  reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuffers.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd_, VIDIOC_REQBUFS, &reqbuffers)) {
    logger_->error("Unable to configure buffers. Errno: {}", errno);
    exit(EXIT_FAILURE);
  }

  if (reqbuffers.count != buffer_count_) {
    logger_->warn("Driver was only able to allocate {} buffers.", reqbuffers.count);
  }

  buffer_count_ = reqbuffers.count;
  buffers_ = new BufferMap[buffer_count_]; //allocate memory for buffer mapping information
  
  // clear buf maps array in case we need to unmap during the following loop
  // because of failure
  for (unsigned int n = 0; n < buffer_count_; n++) {
    buffers_[n].start_ = nullptr;
    buffers_[n].length_ = 0;
  }


  for (unsigned int n = 0; n < buffer_count_; n++) {
    v4l2_buffer buf = {0};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n;

    if (-1 == xioctl(fd_, VIDIOC_QUERYBUF, &buf)) {
      logger_->error("Querying buffer {} failed.", n);
      exit(EXIT_FAILURE);
    }

    buffers_[n].length_ = buf.length;
    buffers_[n].start_ = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_, buf.m.offset);
    
    if (MAP_FAILED == buffers_[n].start_) {
      logger_->error("Memory mapping failed with errno: {}", errno);
      exit(EXIT_FAILURE);
    }
  }
}

void Webcam::startCapture() {
  v4l2_buf_type type;

  for (unsigned int n = 0; n < buffer_count_; n++) {
    // Queue buffers
    v4l2_buffer buf = {0};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n;

    if (-1 == xioctl(fd_, VIDIOC_QBUF, &buf)) {
      logger_->error("Unable to queue buffer {}. Errno: {}.", n, errno);
      exit(EXIT_FAILURE);
    }
  }

  // enable streaming
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl(fd_, VIDIOC_STREAMON, &type)) {
    logger_->error("Unable to start streaming. Errno: {}.", errno);
    exit(EXIT_FAILURE);
  }
}

void Webcam::endCapture() {
  v4l2_buf_type type;

  if (-1 == xioctl(fd_, VIDIOC_STREAMOFF, &type)) {
    logger_->error("Unable to stop streaming. Errno: {}.");
    exit(EXIT_FAILURE);
  }
}

int Webcam::grabFrame(mgk::Blob& dest_blob) {
  // Refactor?: return a SharedPtr to a CamFrame obj that is composed
  // of a blob and timestamp. Nullptr return on EAGAIN.
  v4l2_buffer buf = {0};
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

unsigned int Webcam::imgHeight() const {
  return imgHeight_;
}

unsigned int Webcam::imgWidth() const {
  return imgWidth_;
}

bool Webcam::checkCapabilities() {
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

void DecodeBar(mgk::Image &img) {
  using namespace zxing;

  auto console = spd::get("console");
  Ref<LuminanceSource> source = MagickSource::create(img);
  std::vector<Ref<Result>> results;

  Ref<Binarizer> binarizer;
  binarizer = new HybridBinarizer(source);
  
  DecodeHints hints(DecodeHints::DEFAULT_HINT);

  Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));

  Ref<Reader> reader(new MultiFormatReader);

  try {
    results = std::vector<Ref<Result>>(1, reader->decode(binary, hints));
  } catch (const ReaderException& e) {
    console->info("Reader Exception: {}", std::string(e.what()));
  }
  

  console->info("Got results vector with length: {}", results.size());
  
  img.fillColor("green");
  img.strokeColor("green");
  img.strokeWidth(1);

  for (unsigned int i = 0; i < results.size(); i++) {
    for (int j = 0; j < results[i]->getResultPoints()->size(); j++) {
      int x = results[i]->getResultPoints()[j]->getX();
      int y = results[i]->getResultPoints()[j]->getY();
      img.draw(mgk::DrawableCircle(x, y, x+10, y));
    }

    console->info("Text: {}", results[i]->getText()->getText());
  }
}

int main(int argc, char** argv) {
  mgk::InitializeMagick(*argv);
  auto console = spdlog::stdout_color_mt("console");
  console->info("Welcome to stdlog!");
  auto v4l = spdlog::stdout_color_mt("v4l");
  std::string d = "/dev/video0";
  Webcam v(d);
  v.init();
  v.startCapture();

  mgk::Blob blob;
  
  int frameCount = 0;

  while (frameCount < 60) {
    fd_set fds;
    FD_ZERO(&fds);
    int webcam_fd = v.fd();
    FD_SET(webcam_fd, &fds);

    timeval tout = {1, 0};
    
    int ret = select(webcam_fd+1, &fds, NULL, NULL, &tout);

    switch(ret) {
      case -1:
        console->error("select() call error. Errno: {}", errno);
        exit(EXIT_FAILURE);
      case 0:
        console->warn("Timeout on select() call.");
        continue; // next iteration
    }
  
    ret = v.grabFrame(blob);
    
    if (!ret) continue; // frame was not ready
    
    // blob contains a frame
    frameCount++;
    console->info("Frame: {}", frameCount);

    mgk::Image img(blob, mgk::Geometry(v.imgWidth(), v.imgHeight()));

    std::ostringstream ss;
    ss << "Frame_" << frameCount << ".jpg";
    DecodeBar(img);
    img.write(ss.str());
  }

  v.endCapture();
  v.close();
  return 0;
}
