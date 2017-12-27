#ifndef READER_H_
#define READER_H_

#include <string>
#include <memory>

#include "frame.h"

namespace ZXing {
  class MultiFormatReader;
}

struct ScanResult {
  std::string format_;
  std::string text_;
};

class BarcodeReader {
  public:
    explicit BarcodeReader(bool try_harder = true, bool try_rotate = true); 

    ScanResult scan(FramePtr frame);
  private:
    std::shared_ptr<ZXing::MultiFormatReader> reader_;
};

#endif
