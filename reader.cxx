#include "reader.h"

#include "TextUtfEncoding.h"
#include "GenericLuminanceSource.h"
#include "HybridBinarizer.h"
#include "BinaryBitmap.h"
#include "MultiFormatReader.h"
#include "Result.h"
#include "DecodeHints.h"

using namespace ZXing;

BarcodeReader::BarcodeReader(bool tryHarder, bool tryRotate) {
  DecodeHints hints;
  hints.setShouldTryHarder(tryHarder);
  hints.setShouldTryRotate(tryRotate);
  hints.setPossibleFormats({BarcodeFormat::EAN_8, BarcodeFormat::EAN_13,
                                BarcodeFormat::QR_CODE});

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
    return ScanResult{ToString(result.format()), text};
  }

  return ScanResult();
}
