#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
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

  void startStream(const std::string &url);
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

  void decodeLoopFFmpeg(const std::string &url);
  bool tryOnceFFmpeg(const std::string &url);
  void cleanupFFmpeg();
};
