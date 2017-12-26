#ifndef FRAME_QUEUE_H_
#define FRAME_QUEUE_H_

#include "frame.h"

#include <queue>
#include <mutex>
#include <condition_variable>

/* Thread-safe queue of frames for zxing to process */
class FrameQueue {
  public:
    FrameQueue();
    virtual ~FrameQueue();

    void push(FramePtr p);
    FramePtr wait_and_pop();
    int size() const;
  protected:
    mutable std::mutex mutex_;
    std::queue<FramePtr> data_;
    std::condition_variable cond_;
};


#endif
