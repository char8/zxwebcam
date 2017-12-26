#ifndef DECODE_THREAD_H_
#define DECODE_THREAD_H_
#include "frame.h"
#include "frame_queue.h"

#include <atomic>

void decode_thread(ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag);
  

#endif
