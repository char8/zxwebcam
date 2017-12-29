#ifndef FRAME_QUEUE_H_
#define FRAME_QUEUE_H_

#include "frame.h"

#include <queue>
#include <mutex>
#include <condition_variable>

/* Thread-safe queue of frames for zxing to process */

template<typename T>
class ThreadsafeQueue {
  public:
    ThreadsafeQueue() {};
    virtual ~ThreadsafeQueue() {};

    void push(T p) {
      std::lock_guard<std::mutex> lock(mutex_);
      data_.push(p);
      cond_.notify_one();
    };

    FramePtr wait_and_pop() {
      std::unique_lock<std::mutex> lock(mutex_);
      while(data_.empty()) {
        cond_.wait(lock);
      }

      T p = data_.front();
      data_.pop();
      return p;
    };

    FramePtr pop_with_timeout(int timeout) {
      std::unique_lock<std::mutex> lock(mutex_);
      while(data_.empty()) {
        auto r = cond_.wait_for(lock, std::chrono::seconds(timeout));
        if (r == std::cv_status::timeout) {
          return nullptr;
        }
      }

      T p = data_.front();
      data_.pop();
      return p;
    };

    int size() const {
      std::lock_guard<std::mutex> lock(mutex_);
      return data_.size();
    };
  protected:
    mutable std::mutex mutex_;
    std::queue<T> data_;
    std::condition_variable cond_;
};


#endif
