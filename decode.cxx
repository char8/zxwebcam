

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
