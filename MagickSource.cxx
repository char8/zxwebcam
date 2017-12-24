#include "MagickSource.h"

namespace mgk = Magick;

zxing::Ref<zxing::LuminanceSource> MagickSource::create(mgk::Image &source) {
  return zxing::Ref<zxing::LuminanceSource>(new MagickSource(source));
}

MagickSource::MagickSource(mgk::Image &source):
  zxing::LuminanceSource(source.columns(), source.rows()),
  image_(source) {
    // Set the colorspace to greyscale if it is not already and quantize to 256col
    if (image_.colorSpace() != mgk::GRAYColorspace) {
      image_.quantizeColorSpace(mgk::GRAYColorspace);
      image_.quantizeColors(256);
      image_.quantize();
    }
  }

zxing::ArrayRef<char> MagickSource::getRow(int y, zxing::ArrayRef<char> row) const {
  
  // create a row if one not sent
  if (!row) {
    row = zxing::ArrayRef<char>(getWidth());
  }
  
  // write to underlying char array using imagemagick API
  // cast away const mgk::Image, call should be safe as image_ not being modified by write
  static_cast<mgk::Image>(image_).write(0, y, getWidth(), 1, "R", mgk::CharPixel, &row[0]);

  return row;
}

zxing::ArrayRef<char> MagickSource::getMatrix() const {
  zxing::ArrayRef<char> mat = zxing::ArrayRef<char>(getWidth() * getHeight());
  
  // cast away const mgk::Image, call should be safe as image_ not being modified by write
  static_cast<mgk::Image>(image_).write(0, 0, getWidth(), getHeight(), "R", mgk::CharPixel, &mat[0]);

  return mat;
}
