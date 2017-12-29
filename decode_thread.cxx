#include "reader.h"
#include "decode_thread.h"
#include "frame.h"
#include "threadsafe_queue.h"

#include <spdlog/spdlog.h>
#include <jpeglib.h>
#include <jerror.h>
#define cimg_plugin "plugins/jpeg_buffer.h"
#include <CImg.h>

#include <atomic>
#include <vector>
#include <cstdio>

using namespace cimg_library;

void decode_thread(DecoderSetup ds, ThreadsafeQueue<FramePtr>& queue,
                   std::atomic_bool& exit_flag) {
  
  auto logger = spdlog::get("console");
  
#ifdef XDISPLAY
  CImgDisplay main_disp(ds.res_x_, ds.res_y_, "barcode preview", 3, false, true);
  if (ds.enable_preview_) {
    main_disp.show();
  }
#else
  if (ds.enable_preview_) {
    logger->warn("Not compiled with X11 support - cannot show preview");
  }
#endif

  std::vector<ZXing::BarcodeFormat> fmts;
  for(auto& f: ds.formats_) {
    auto bf = ZXing::BarcodeFormatFromString(f);
    if (bf == ZXing::BarcodeFormat::FORMAT_COUNT) {
      logger->error("Invalid format {}", f);
    } else {
      logger->info("Scanning for barcode format: {}", f);
    }
    fmts.push_back(bf);
  }

  BarcodeReader br(fmts);

  
  while(true) {
    FramePtr p = queue.pop_with_timeout(1);
    
    if (exit_flag) { break; } // exit flag
    if (p == nullptr) { continue; } // timeout

#ifdef XDISPLAY
    if (ds.enable_preview_) {
      CImg<unsigned char> img(p->buf(), 3, p->cols(), p->rows());
      // CImg needs a different byte order
      img.permute_axes("YZCX");
      img.display(main_disp);
      logger->debug("preview");
    }
#endif

    auto res = br.scan(p);
    
    if (!res.text_.empty()) {
      logger->info(res.text_);
      logger->info(res.format_);
    }
  }
}
