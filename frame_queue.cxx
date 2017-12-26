#include "frame_queue.h"

FrameQueue::FrameQueue() {}

FrameQueue::~FrameQueue() {}

void FrameQueue::push(FramePtr p) {
  std::lock_guard<std::mutex> lock(mutex_);
  data_.push(p);
  cond_.notify_one();
}

FramePtr FrameQueue::wait_and_pop() {
  std::unique_lock<std::mutex> lock(mutex_);
  while(data_.empty()) {
    cond_.wait(lock);
  }

  FramePtr p = data_.front();
  data_.pop();
  return p;
}

int FrameQueue::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return data_.size();
}

