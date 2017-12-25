#ifndef FRAME_H_
#define FRAME_H_

#include <cstddef>

using std::size_t;
/* Represent a capture frame from the webcam */

enum class FrameEncoding {
  JPEG
};

class Frame {
public:
  Frame(char* source, FrameEncoding enc, size_t bytes): enc_(enc),
        buffer_(new char[bytes]), buf_length_(bytes) {
    // copy the frame contents
    for(unsigned int i = 0; i < bytes; i++) {
      buffer_[i] = source[i];
    }
  }
  
  virtual ~Frame() {
    delete[] buffer_;
    buffer_ = nullptr;
  }

  char* buf() { return buffer_; };
  size_t buflen() { return buf_length_; }
  FrameEncoding enc() { return enc_; };

protected:
  FrameEncoding enc_;
  char* buffer_;
  size_t buf_length_;
};

#endif
