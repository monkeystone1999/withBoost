#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class Video {
public:
  struct FramePayload {
    const uint8_t *dataY = nullptr;
    const uint8_t *dataUV = nullptr;
    int width = 0;
    int height = 0;
    int strideY = 0;
    int strideUV = 0;
  };

  using FrameCallback = std::function<void(const FramePayload &)>;

  explicit Video() = default;
  ~Video();

  void startStream(const std::string &url, int fpsLimit);

  // Dynamic FPS update — safe to call from any thread while streaming.
  void setFpsLimit(int fps) {
    fpsLimitMs_.store(fps > 0 ? 1000 / fps : 33, std::memory_order_relaxed);
  }
  void stopStream();

  FrameCallback onFrameReady;

private:
  AVFormatContext *formatCtx_ = nullptr;
  AVCodecContext *codecCtx_ = nullptr;
  int videoStreamIndex_ = -1;

  std::thread decodeThread_;
  std::atomic<bool> stopThread_{false};

  // Frame timing
  std::chrono::steady_clock::time_point lastFrameTime_;
  std::atomic<int> fpsLimitMs_{33};

  bool isStopping() const {
    return stopThread_.load(std::memory_order_relaxed);
  }

  static int interrupt_cb(void *ctx);

  void decodeLoopFFmpeg(const std::string &url);
  bool tryOnceFFmpeg(const std::string &url);
  void cleanupFFmpeg();
};
