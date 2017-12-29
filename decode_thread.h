#ifndef DECODE_THREAD_H_
#define DECODE_THREAD_H_
#include "frame.h"
#include "threadsafe_queue.h"

#include <atomic>
#include <string>
#include <vector>

const int BACKOFF_SECS = 2;

struct ScanResult;

struct DecoderSetup {
  std::vector<std::string> formats_;
  bool enable_preview_;
  unsigned int res_x_;
  unsigned int res_y_;
};

void decode_thread(DecoderSetup ds,
                   ThreadsafeQueue<FramePtr>& frame_queue,
                   ThreadsafeQueue<ScanResult>& result_queue,
                   std::atomic_bool& exit_flag);
  

#endif
