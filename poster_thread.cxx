#include "poster_thread.h"
#include "reader.h"

#include <spdlog/spdlog.h>
#include <jpeglib.h>
#include <jerror.h>
#define cimg_plugin "plugins/jpeg_buffer.h"
#include <CImg.h>
#include <cpr/cpr.h>
#include <msgpack.hpp>

using namespace cimg_library;

const unsigned char GREEN[] = {0, 255, 0};


void poster_thread(std::string url_string,
                   ThreadsafeQueue<ScanResult>& result_queue,
                   std::atomic_bool& exit_flag) {
  
  auto logger = spdlog::get("console");
  while(!exit_flag) {
    auto r = result_queue.pop_with_timeout(1);
    if ((r.frame_ == nullptr) || (url_string.empty())) { continue; }

    // convert into image, draw result points, and encode as JPEG
    CImg<unsigned char> frame(r.frame_->buf(), 3, r.frame_->cols(),
                              r.frame_->rows());
    frame.permute_axes("YZCX");
    
    for (auto& rp : r.result_points_) {
      frame.draw_circle(rp.first, rp.second,
                        10,
                        GREEN);
    }
    
    // should be smaller than raw bitmap
    unsigned int jpeg_size = r.frame_->buflen();
    JOCTET* jpeg_out = new JOCTET[jpeg_size];
    frame.save_jpeg_buffer(jpeg_out, jpeg_size, 60);
    
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, r.text_);
    msgpack::pack(sbuf, r.format_);
    msgpack::pack(sbuf, r.result_points_);
    msgpack::pack(sbuf, msgpack::type::raw_ref(reinterpret_cast<char*>(jpeg_out),
                  jpeg_size));

    logger->debug("jpeg {} bytes, msgpack {} bytes", jpeg_size, sbuf.size());

    auto post = cpr::Post(cpr::Url(url_string),
          cpr::Body{sbuf.data(), sbuf.size()},
          cpr::Timeout{1000});

    if (!post.error.message.empty()) {
      logger->warn("Could not post result: {}", post.error.message);
    }
    delete[] jpeg_out;
    

  }
}

