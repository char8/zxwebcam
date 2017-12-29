#ifndef DECODE_THREAD_H_
#define DECODE_THREAD_H_
#include "frame.h"
#include "threadsafe_queue.h"

#include "ResultPoint.h"

#include <atomic>
#include <string>
#include <vector>

struct DecoderSetup {
  std::vector<std::string> formats_;
  bool enable_preview_;
  unsigned int res_x_;
  unsigned int res_y_;
};

void decode_thread(DecoderSetup ds, ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag);
  

#endif
