#ifndef POSTER_THREAD_H_
#define POSTER_THREAD_H_

#include <atomic>
#include <string>

#include "threadsafe_queue.h"

static const int TIMEOUT_SECONDS = 1;

struct ScanResult;

void poster_thread(std::string url_string,
                   ThreadsafeQueue<ScanResult>& result_queue,
                   std::atomic_bool& exit_flag);

#endif
