#include "reader.h"
#include "decode_thread.h"

#include <spdlog/spdlog.h>
#include <CImg.h>

#include <chrono>
#include <atomic>
#include <vector>
#include <cstdio>

using namespace cimg_library;

void decode_thread(DecoderSetup ds,
                   ThreadsafeQueue<FramePtr>& frame_queue,
                   ThreadsafeQueue<ScanResult>& result_queue,
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
  
  auto last_post_time = std::chrono::steady_clock::now();
  auto last_scan_time = std::chrono::steady_clock::now();
  std::string last_scan;
  
  while(true) {
    FramePtr p = frame_queue.pop_with_timeout(1);
    
    if (exit_flag) { break; } // exit flag
    if (p == nullptr) { continue; } // timeout

    auto res = br.scan(p);
    
    auto now_time = std::chrono::steady_clock::now();

    // post results provided backoff conditions met
    if (!res.text_.empty()) {
      // only post the result if we're backoff_seconds_ from the same result
      // last being scanned
      bool b = ((now_time - last_scan_time) <= std::chrono::seconds{BACKOFF_SECS});
      if ((res.text_ != last_scan) || (!b)) {
        result_queue.push(res);
      } else {
        logger->debug("Dropping result due to backoff");
      }

      last_scan = res.text_;
      last_scan_time = now_time;
      last_post_time = now_time;
    }

    // periodically post images, preview
    if (now_time - last_post_time > std::chrono::seconds{1}) {
      last_post_time = now_time;
      if (result_queue.size() < 5) {
        result_queue.push(res);
      }
    }

#ifdef XDISPLAY
    if (ds.enable_preview_) {
      CImg<unsigned char> img(p->buf(), 3, p->cols(), p->rows());
      // CImg needs a different byte order
      img.permute_axes("YZCX");
      img.display(main_disp);
    }
#endif

  }
}
