#include "webcam.h"

#include <sstream>
#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
  auto console = spdlog::stdout_color_mt("console");
  console->info("Welcome to stdlog!");
  auto v4l = spdlog::stdout_color_mt("v4l");
  std::string d = "/dev/video1";
  zxwebcam::Webcam v(d);
  v.init();
  v.start_capture();

  
  int frameCount = 0;

  while (frameCount < 60) {
    fd_set fds;
    FD_ZERO(&fds);
    int webcam_fd = v.fd();
    FD_SET(webcam_fd, &fds);

    timeval tout = {1, 0};
    
    int ret = select(webcam_fd+1, &fds, NULL, NULL, &tout);

    switch(ret) {
      case -1:
        console->error("select() call error. Errno: {}", errno);
        exit(EXIT_FAILURE);
      case 0:
        console->warn("Timeout on select() call.");
        continue; // next iteration
    }
  
    auto sp = v.grab_frame();
    
    if (sp == nullptr) continue; // frame was not ready
    
    // blob contains a frame
    frameCount++;
    console->info("Frame: {}", frameCount);

    std::ostringstream ss;
    ss << "Frame_" << frameCount << ".jpg";
    //DecodeBar(img);
    //img.display();
    //img.write(ss.str());
  }

  v.end_capture();
  v.close();
  return 0;
}
