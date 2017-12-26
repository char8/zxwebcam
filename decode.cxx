#include "frame.h"
#include "frame_queue.h"
#include "frame_source.h"

#include <zxing/Binarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/ReaderException.h>
#include <zxing/Result.h>
#include <zxing/Exception.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>
#include <zxing/MultiFormatReader.h>

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
  
  CImg<unsigned char> temp(640, 480, 1, 3, 0);

  CImgDisplay main_disp(temp, "your barcode sir");
  CImgDisplay sec_disp(temp, "your binary sir");
  
  using namespace zxing;
  auto logger = spdlog::get("console");
  
  while(true) {
    FramePtr p = queue.wait_and_pop();
    
    p->convert_to_greyscale();
    // break loop if queue poisoned or exit_flag set
    if ((p == nullptr) || (exit_flag))
      break;

    /*
    unsigned int dim = p->rows()*p->cols();
    unsigned char buf2[3*dim];
    for(unsigned int i = 0, j = 0; i < p->buflen(); i += 3, j++) {
      buf2[j] = p->buf()[i];
      buf2[dim+j] = p->buf()[i+1];
      buf2[dim*2+j] = p->buf()[i+2];
    }
    */

    CImg<unsigned char> img(p->buf(), p->cols(), p->rows(), 1, 1);
    img.display(main_disp);
    
    std::vector<Ref<Result>> results;
    auto source = FrameSource::create(p);


    Ref<Binarizer> binarizer;
    binarizer = new HybridBinarizer(source);

    DecodeHints hints(DecodeHints::DEFAULT_HINT);
    hints.setTryHarder(false);
    Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));
    
    for(int r = 0; r < p->rows(); r++) {
      for(int c = 0; c < p->cols(); c++) {
        img.atXYZ(c, r, 0, 0) = binary->getBlackMatrix()->get(c, r);
      }
    }

    img.display(sec_disp);
    Ref<Reader> reader(new MultiFormatReader);

    try {
      results = std::vector<Ref<Result>>(1, reader->decode(binary, hints));
    } catch (const ReaderException& e) {
      logger->debug("Reader Exception: {}", e.what());
    } catch (const IllegalArgumentException& e) {
    }
    
    logger->info("Got: {}", results.size());
    for (auto it = results.begin(); it != results.end(); ++it) {
      logger->info("Text: {}", (*it)->getText()->getText());
    }
  }

}

// void DecodeBar(mgk::Image &img) {
//   using namespace zxing;
// 
//   auto console = spd::get("console");
//   Ref<LuminanceSource> source = MagickSource::create(img);
//   std::vector<Ref<Result>> results;
// 
//   Ref<Binarizer> binarizer;
//   binarizer = new HybridBinarizer(source);
//   
//   DecodeHints hints(DecodeHints::DEFAULT_HINT);
//   hints.setTryHarder(false);
//   Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));
// 
//   Ref<Reader> reader(new MultiFormatReader);
// 
//   try {
//     results = std::vector<Ref<Result>>(1, reader->decode(binary, hints));
//   } catch (const ReaderException& e) {
//  //   console->info("Reader Exception: {}", std::string(e.what()));
//   }
//   
// 
//   //console->info("Got results vector with length: {}", results.size());
//   
//   img.fillColor("green");
//   img.strokeColor("green");
//   img.strokeWidth(1);
// 
//   for (unsigned int i = 0; i < results.size(); i++) {
//     for (int j = 0; j < results[i]->getResultPoints()->size(); j++) {
//       int x = results[i]->getResultPoints()[j]->getX();
//       int y = results[i]->getResultPoints()[j]->getY();
//       img.draw(mgk::DrawableCircle(x, y, x+10, y));
//     }
// 
//     console->info("Text: {}", results[i]->getText()->getText());
//   }
// }
// 
// int main(int argc, char** argv) {
//   mgk::InitializeMagick(*argv);
//   auto console = spdlog::stdout_color_mt("console");
//   console->info("Welcome to stdlog!");
//   auto v4l = spdlog::stdout_color_mt("v4l");
//   std::string d = "/dev/video0";
//   Webcam v(d);
//   v.init();
//   v.startCapture();
// 
//   mgk::Blob blob;
//   
//   int frameCount = 0;
// 
//   while (frameCount < 60) {
//     fd_set fds;
//     FD_ZERO(&fds);
//     int webcam_fd = v.fd();
//     FD_SET(webcam_fd, &fds);
// 
//     timeval tout = {1, 0};
//     
//     int ret = select(webcam_fd+1, &fds, NULL, NULL, &tout);
// 
//     switch(ret) {
//       case -1:
//         console->error("select() call error. Errno: {}", errno);
//         exit(EXIT_FAILURE);
//       case 0:
//         console->warn("Timeout on select() call.");
//         continue; // next iteration
//     }
//   
//     ret = v.grabFrame(blob);
//     
//     if (!ret) continue; // frame was not ready
//     
//     // blob contains a frame
//     frameCount++;
//     console->info("Frame: {}", frameCount);
// 
//     mgk::Image img(blob, mgk::Geometry(v.imgWidth(), v.imgHeight()));
// 
//     std::ostringstream ss;
//     ss << "Frame_" << frameCount << ".jpg";
//     DecodeBar(img);
//     img.display();
//     img.write(ss.str());
//   }
// 
//   v.endCapture();
//   v.close();
//   return 0;
// }
