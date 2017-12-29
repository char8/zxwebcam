#include "reader.h"

#include "TextUtfEncoding.h"
#include "GenericLuminanceSource.h"
#include "HybridBinarizer.h"
#include "BinaryBitmap.h"
#include "MultiFormatReader.h"
#include "Result.h"
#include "DecodeHints.h"

using namespace ZXing;

BarcodeReader::BarcodeReader(std::vector<BarcodeFormat> fmts, 
                             bool tryHarder, bool tryRotate) {
  DecodeHints hints;
  hints.setShouldTryHarder(tryHarder);
  hints.setShouldTryRotate(tryRotate);
  hints.setPossibleFormats(fmts);

  reader_ = std::make_shared<MultiFormatReader>(hints);
}

static std::shared_ptr<LuminanceSource> CreateLuminanceSource(FramePtr frame) {
  return std::make_shared<GenericLuminanceSource>(frame->cols(),
                frame->rows(),
                frame->buf(),
                frame->cols()*3,
                3, 2, 1, 0);
}

static std::shared_ptr<BinaryBitmap> CreateBinaryBitmap(FramePtr frame) {
  return std::make_shared<HybridBinarizer>(CreateLuminanceSource(frame));
}

ScanResult BarcodeReader::scan(FramePtr f) {
  Result result(DecodeStatus::NotFound);
  
  auto bin = CreateBinaryBitmap(f);

  result = reader_->read(*bin);
  
  if (result.isValid()) {
    std::string text;
    TextUtfEncoding::ToUtf8(result.text(), text);
    std::vector<std::pair<int,int>> v;
    for (auto& rp : result.resultPoints()) {
      v.push_back({static_cast<int>(rp.x()), static_cast<int>(rp.y())});
    }
    return ScanResult{f,
                      ToString(result.format()),
                      text,
                      v};
  }

  auto sr = ScanResult();
  sr.frame_ = f;
  return sr;
}
