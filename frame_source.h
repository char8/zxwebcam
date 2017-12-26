#ifndef FRAME_SOURCE_H_
#define FRAME_SOURCE_H_

#include "frame.h"

#include <zxing/LuminanceSource.h>

class FrameSource : public zxing::LuminanceSource {
public:
  // factory function
  static zxing::Ref<zxing::LuminanceSource> create(FramePtr& source);

  FrameSource(FramePtr& source);

  zxing::ArrayRef<char> getRow(int y, zxing::ArrayRef<char> row) const;
  zxing::ArrayRef<char> getMatrix() const;
protected:
  FramePtr source_;
};

#endif
