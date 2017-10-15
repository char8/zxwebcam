#ifndef __MAGICK_SOURCE_H__
#define __MAGICK_SOURCE_H__

#include <zxing/LuminanceSource.h>
#include <Magick++.h>

namespace mgk = Magick;

class MagickSource : public zxing::LuminanceSource {
private:
  mgk::Image image_;
public:
  static zxing::Ref<zxing::LuminanceSource> create(mgk::Image &source);

  MagickSource(mgk::Image &source);

  zxing::ArrayRef<char> getRow(int y, zxing::ArrayRef<char> row) const;
  zxing::ArrayRef<char> getMatrix() const;

  // TODO: find a way to test!
  //  bool isCropSupported() const;
  //  Ref<zxing::LuminanceSource> crop(int left, int top, int width, int height) const;
  //
  //  bool isRotateSupported() const;
  //  Ref<zxing::LuminanceSource> rotateCounterClockwise() const;
};

#endif
