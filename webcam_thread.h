#ifndef WEBCAM_THREAD_H_
#define WEBCAM_THREAD_H_

#include "frame.h"
#include "frame_queue.h"

#include <string>
#include <atomic>

void webcam_thread(std::string device, ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag);
 

#endif
