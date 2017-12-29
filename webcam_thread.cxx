#include "webcam.h"
#include "webcam_thread.h"
#include "frame.h"
#include "threadsafe_queue.h"

#include <chrono>
#include <string>
#include <atomic>
#include <system_error>
#include <stdexcept>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include <spdlog/spdlog.h>

void webcam_thread(WebcamSetup ws, ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag) {

  auto logger = spdlog::get("console");
  
  // FPS measurement and some metrics
  int frame_count = 0;
  int dropped_frames = 0;
  const int fps_div_sb = 3; // divide by shifting 3 bit pos (/8)
  auto fps_log_seconds = std::chrono::seconds{1 << fps_div_sb};
  std::chrono::time_point<std::chrono::steady_clock>  start_time = \
                std::chrono::steady_clock::now(), now_time;


  zxwebcam::Webcam v(ws.device_, ws.res_y_, ws.res_x_, ws.fps_, ws.fps_);
  
  logger->info("Initialising webcam {} with res {}x{} @ {} fps",
               ws.device_, ws.res_x_, ws.res_y_, ws.fps_);
  try {
    v.init();
  } catch (const std::runtime_error& e) {
    logger->error("Could not initialise webcam device: <{}>",
                  e.what());
    exit_flag = true;
    return;
  }

  try {
    v.start_capture();
  } catch (const std::runtime_error& e) {
    logger->error("Could not start capture: <{}>",
                  e.what());
    exit_flag = true;
    return;
  }

  while (!exit_flag) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(v.fd(), &fds);

    timeval timeout = {1, 0}; // 1 sec timeout
    int ret = select(v.fd()+1, &fds, NULL, NULL, &timeout);

    switch(ret) {
      case -1:
        logger->error("select() call error with errno {}", errno);
        return;
      case 0:
        logger->warn("timeout waiting for frame from webcam.");
        continue;
      default:
        break;
    }

    FramePtr f = v.grab_frame();
  
    if (f == nullptr)
      continue;
    

    //TODO: put the hardcoded magic num somewhere sensible
    if (queue.size() > (int)(ws.fps_)) {
      logger->warn("Frame queue full, discarding frame.");
      dropped_frames++;
    } else {
      queue.push(f);
      frame_count++;
    }

    now_time = std::chrono::steady_clock::now();

    if (now_time - start_time >= fps_log_seconds) {
      // log fps
      logger->info("frames {}, dropped {} frames, avg fps {}",
          frame_count,
          dropped_frames,
          (frame_count >> fps_div_sb));

      frame_count = 0;
      start_time = now_time;
    }
  }
  
  try {
    v.end_capture();
    v.close();
  } catch (const std::runtime_error& e) {
    logger->error("Could not shutdown webcam device: {}",
                  e.what());
    exit_flag = true;
  }
}
