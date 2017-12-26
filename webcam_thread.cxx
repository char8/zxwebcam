#include "webcam.h"
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

void webcam_thread(std::string device, ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag) {

  auto logger = spdlog::get("console");
  
  zxwebcam::Webcam v(device);
  
  try {
    v.init();
  } catch (const std::system_error& e) {
    logger->error("Could not initialise webcam device: <{}> {}",
                  e.what(),
                  e.code().message());
    return;
  } catch (const std::runtime_error& e) {
    logger->error("Could not initialise webcam device: {}",
                  e.what());
    return;
  }

  try {
    v.start_capture();
  } catch (const std::system_error& e) {
    logger->error("Could not start capture: <{}> {}",
                  e.what(),
                  e.code().message());
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
    if (queue.size() > 10) {
      logger->warn("Frame queue full, discarding frame.");
    } else {
      queue.push(f);
    }
  }
  
  try {
    v.end_capture();
    v.close();
  } catch (const std::system_error& e) {
    logger->error("Could not shutdown webcam device: <{}> {}",
                  e.what(),
                  e.code().message());
  } catch (const std::runtime_error& e) {
    logger->error("Could not shutdown webcam device: {}",
                  e.what());
  }
}
