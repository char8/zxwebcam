#include "webcam.h"
#include "webcam_thread.h"
#include "frame.h"
#include "frame_queue.h"

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
  
  zxwebcam::Webcam v(ws.device_, ws.res_y_, ws.res_x_, ws.fps_);
  
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
    
    //logger->debug("Frame captured");

    //TODO: put the hardcoded magic num somewhere sensible
    if (queue.size() > (int)(ws.fps_)) {
      logger->warn("Frame queue full, discarding frame.");
    } else {
      queue.push(f);
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
