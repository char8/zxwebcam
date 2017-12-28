#include "reader.h"
#include "frame.h"
#include "frame_queue.h"

#include <spdlog/spdlog.h>

#include <atomic>
#include <vector>

#include <cstdio>
#include <jpeglib.h>
#include <jerror.h>

#define cimg_plugin "plugins/jpeg_buffer.h"
#include <CImg.h>

using namespace cimg_library;

void decode_thread(ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag) {

#ifdef XDISPLAY
  CImgDisplay main_disp(800, 600, "barcode");
#endif

  BarcodeReader br;

  auto logger = spdlog::get("console");
  
  while(true) {
    FramePtr p = queue.wait_and_pop();
    
    // break loop if queue poisoned or exit_flag set
    if ((p == nullptr) || (exit_flag))
      break;

#ifdef XDISPLAY
    CImg<unsigned char> img(p->buf(), 3, p->cols(), p->rows());
    img.permute_axes("YZCX");
    img.display(main_disp);
#endif

    auto res = br.scan(p);
    
    if (!res.text_.empty()) {
      logger->info(res.text_);
      logger->info(res.format_);
    }
  }
}
