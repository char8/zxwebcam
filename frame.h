#ifndef FRAME_H_
#define FRAME_H_

#include <cstddef>
#include <memory>
#include <cassert>

using std::size_t;

class Frame;

using FramePtr = std::shared_ptr<Frame>;

enum class FrameFormat {
  RGB24,
  GREY8
};

/* Represent a capture frame from the webcam */
class Frame {
  public:
    Frame(const unsigned char* source, size_t bytes, unsigned int rows, unsigned int cols,
          FrameFormat format):
          buffer_(new unsigned char[bytes]),
          buf_length_(bytes),
          rows_(rows),
          cols_(cols),
          format_(format) {

      // copy the frame contents from source
      for(unsigned int i = 0; i < bytes; i++) {
        buffer_[i] = source[i];
      }
    }
    
    virtual ~Frame() {
      delete[] buffer_;
      buffer_ = nullptr;
    }

    const unsigned char* buf() const { return buffer_; }
    size_t buflen() const { return buf_length_; }
    
    unsigned int rows() const { return rows_; }
    unsigned int cols() const { return cols_; }
    FrameFormat format() const { return format_; }

    void convert_to_greyscale() {
      int tmp = 0;
      if (format_ == FrameFormat::GREY8) return; // do nothing, already 1byte/pix = grey
      
      size_t new_bytes = (size_t)(rows()*cols());
      unsigned char* new_buf = new unsigned char[new_bytes];

      for(unsigned int i = 0, j = 0; i < buf_length_; i += 3, j++) {
        tmp = buffer_[i] << 1; // R*2
        tmp += (buffer_[i+1] << 2) + buffer_[i+1]; // G*5
        tmp += (buffer_[i+2]); // B*1
        
        assert(j < new_bytes);
        assert((i+2) < buf_length_);

        new_buf[j] = (char)(tmp >> 3);
      }

      delete[] buffer_;
      buffer_ = new_buf;
      buf_length_ = new_bytes;
      format_ = FrameFormat::GREY8;
    }

  protected:
    unsigned char* buffer_;
    size_t buf_length_;
    unsigned int rows_;
    unsigned int cols_;
    FrameFormat format_;
};

#endif
