#ifndef READER_H_
#define READER_H_

#include "frame.h"

#include "BarcodeFormat.h"

#include <string>
#include <memory>
#include <vector>
#include <utility>

namespace ZXing {
  class MultiFormatReader;
}

struct ScanResult {
  FramePtr frame_;
  std::string format_;
  std::string text_;
  std::vector<std::pair<int,int>> result_points_;
};

class BarcodeReader {
  public:
    explicit BarcodeReader(std::vector<ZXing::BarcodeFormat> fmts,
          bool try_harder = true, bool try_rotate = true); 

    ScanResult scan(FramePtr frame);
  private:
    std::shared_ptr<ZXing::MultiFormatReader> reader_;
};

#endif
