#include "frame_source.h"
#include <cassert>

zxing::Ref<zxing::LuminanceSource> FrameSource::create(FramePtr& source) {
  return zxing::Ref<zxing::LuminanceSource>(new FrameSource(source));
}

FrameSource::FrameSource(FramePtr& source):
    zxing::LuminanceSource(source->cols(), source->rows()),
    source_(source) {
  // convert this frame to grey8
  source_->convert_to_greyscale();
}

zxing::ArrayRef<char> FrameSource::getRow(int y, zxing::ArrayRef<char> row) const {
  if (!row) {
    row = zxing::ArrayRef<char>(getWidth());
  }
  
  assert(source_->format() == FrameFormat::GREY8);
  const char* buf = (const char*)(source_->buf());
  // offset by the y coord
  buf += (getWidth()*y);

  for(int i = 0; i < getWidth(); i++) {
    row[i] = buf[i];  
    assert(buf < (const char*)(source_->buf() + source_->buflen()));
  }
  
  return row;
}

zxing::ArrayRef<char> FrameSource::getMatrix() const {
  zxing::ArrayRef<char> mat = zxing::ArrayRef<char>(getWidth() * getHeight());
  assert((getWidth()*getHeight()) == (int)(source_->buflen()));

  for(unsigned int i = 0; i < source_->buflen(); i++) {
    mat[i] = (source_->buf())[i];
  }

  return mat;
}

