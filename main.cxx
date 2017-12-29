#include "webcam.h"
#include "reader.h"
#include "webcam_thread.h"
#include "decode_thread.h"
#include "poster_thread.h"
#include "threadsafe_queue.h"

#include <spdlog/spdlog.h>
#include <args.hxx>

#include <atomic>
#include <thread>
#include <signal.h>
#include <iostream>

// global flag to exit all threads - set by sighandle
std::atomic_bool exit_flag;

static void handle_signals(int signum);
static void process_barcode_format_flag(args::Flag& f,
                                        std::vector<std::string>& v);

int main(int argc, char** argv) {
  auto console = spdlog::stdout_color_mt("console");

  args::ArgumentParser parser("Barcode reader for webcams",
      "Attempts to read a barcode from a captured image and POST the result");
  
  args::HelpFlag help(parser, "help", "show help", {'h', "help"});
  
  args::Positional<std::string> url(parser, "url", "URL for POSTing results", "");
  args::Positional<std::string> device(parser, "device", "V4L capture device",
                                       "/dev/video0");

  args::ValueFlag<int> res_x(parser, "cap_width", "webcam x pixels", {'x'});
  args::ValueFlag<int> res_y(parser, "cap_height", "webcam y pixels", {'y'});
  args::ValueFlag<int> fps(parser, "fps", "webcam capture framerate", {'r'});

  args::Flag verbose(parser, "verbose", "verbose log output", {'v'});
  args::Flag preview(parser, "preview", "preview video", {'p'});
  args::Group group(parser, "select barcode types to attempt decoding",
                    args::Group::Validators::AtLeastOne);
  // TODO: implement something more DRY...
  args::Flag fmt_codabar(group, "CODABAR", "Support Codabar 1D", {"codabar"});
  args::Flag fmt_code39(group, "CODE_39", "Support Code39 1D", {"code39"});
  args::Flag fmt_code128(group, "CODE_128", "Support Code128 1D", {"code128"});
  args::Flag fmt_ean8(group, "EAN_8", "Support EAN8 1D", {"ean8"});
  args::Flag fmt_ean13(group, "EAN_13", "Support EAN13 1D", {"ean13"});
  args::Flag fmt_qr(group, "QR_CODE", "Support QR 2D", {"qr"});

  try {
    parser.ParseCLI(argc, argv);
  } catch (const args::Help& e) {
    std::cout << parser;
    return 0;
  } catch (const args::ParseError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (const args::ValidationError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  std::vector<std::string> formats;
  
  // handle cmdline args
  process_barcode_format_flag(fmt_codabar, formats);
  process_barcode_format_flag(fmt_code39, formats);
  process_barcode_format_flag(fmt_code128, formats);
  process_barcode_format_flag(fmt_ean8, formats);
  process_barcode_format_flag(fmt_ean13, formats);
  process_barcode_format_flag(fmt_qr, formats);

  WebcamSetup ws {args::get(device), 640, 480, 5};

  if (res_x) { ws.res_x_ = args::get(res_x); }
  if (res_y) { ws.res_y_ = args::get(res_y); }
  if (fps)   { ws.fps_   = args::get(fps);   }
  if (verbose) { console->set_level(spdlog::level::debug); }
  
  DecoderSetup ds {formats, static_cast<bool>(preview), ws.res_x_, ws.res_y_};

  ThreadsafeQueue<FramePtr> frame_queue;
  ThreadsafeQueue<ScanResult> result_queue;

  exit_flag = false;
  
  // insert signal handlers
  struct sigaction sa = {};
  sa.sa_handler = handle_signals;

  if (( sigaction(SIGTERM, &sa, NULL) < 0 ) ||
      ( sigaction(SIGINT , &sa, NULL) < 0 )) {
    console->error("Could not insert signal handlers <{}>", errno);
    return -1;
  }

  std::thread wt(webcam_thread, ws, std::ref(frame_queue), std::ref(exit_flag)); 
  std::thread dt(decode_thread, ds, std::ref(frame_queue),
                 std::ref(result_queue),
                 std::ref(exit_flag));
  std::thread pt(poster_thread, args::get(url), std::ref(result_queue),
                 std::ref(exit_flag));

  wt.join();
  dt.join();
  pt.join();
}

void handle_signals(int signum) {
  std::cerr << "SIGNAL SIGTERM/SIGINT " << signum <<  ": EXITING" << std::endl;
  exit_flag = true;
}

static void process_barcode_format_flag(args::Flag& f,
                                        std::vector<std::string>& v) {
  if (f)
    v.push_back(f.Name());
}

