#include "webcam.h"
#include "webcam_thread.h"
#include "decode_thread.h"
#include "frame_queue.h"
#include "frame.h"

#include <atomic>
#include <thread>
#include <signal.h>

#include <iostream>
#include <spdlog/spdlog.h>

// global flag to exit all threads - set by sighandle
std::atomic_bool exit_flag;

void handle_signals(int signum);

int main(int argc, char** argv) {
  auto console = spdlog::stdout_color_mt("console");
  console->info("Welcome to stdlog!");
  console->set_level(spdlog::level::debug);
  std::string d = "/dev/video0";
  
  ThreadsafeQueue<FramePtr> queue;

  exit_flag = false;

  struct sigaction sa = {};
  sa.sa_handler = handle_signals;

  if (( sigaction(SIGTERM, &sa, NULL) < 0 ) ||
      ( sigaction(SIGINT , &sa, NULL) < 0 )) {
    console->error("Could not insert signal handlers <{}>", errno);
    return -1;
  }

  std::thread wt(webcam_thread, d, std::ref(queue), std::ref(exit_flag)); 
  std::thread dt(decode_thread, std::ref(queue), std::ref(exit_flag));

  wt.join();
  dt.join();
}

void handle_signals(int signum) {
  std::cerr << "SIGNAL SIGTERM/SIGINT: EXITING" << std::endl;
  exit_flag = true;
}
