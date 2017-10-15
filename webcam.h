#ifndef WEBCAM_H__
#define WEBCAM_H__

#include <spdlog/spdlog.h>
#include <Magick++.h>
#include <string>

namespace spd = spdlog;
namespace mgk = Magick;

struct BufferMap {
  void* start_;
  size_t length_;
};

class Webcam {
  private:
    int fd_;
    bool isOpen_; // track v4l device state

    std::string device_;
    std::shared_ptr<spd::logger> logger_;

    unsigned int imgWidth_;
    unsigned int imgHeight_;
    unsigned int fps_;
    unsigned int buffer_count_;
    BufferMap* buffers_;

    bool checkCapabilities();
    void initMmap();
  public:
    Webcam(std::string& device);
    ~Webcam(); 
    void init();

    void close();

    void startCapture();
    void endCapture();
    int grabFrame(mgk::Blob& dest_blob);

    int fd() const;
    unsigned int imgWidth() const;
    unsigned int imgHeight() const;
};

#endif
