#ifndef WEBCAM_THREAD_H_
#define WEBCAM_THREAD_H_

#include "frame.h"
#include "frame_queue.h"

#include <string>
#include <atomic>

struct WebcamSetup {
  std::string device_;
  unsigned int res_x_;
  unsigned int res_y_;
  unsigned int fps_;
};

void webcam_thread(WebcamSetup ws, ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag);
 

#endif
