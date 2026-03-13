#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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
    std::shared_ptr<std::vector<uint8_t>> buffer;
    int width = 0;
    int height = 0;
    int strideY = 0;
    int strideUV = 0;
    int offsetUV = 0;
    int format = 0; // 0: NV12, 1: RGBA
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
  bool loggedFrameInfo_{false};

  std::vector<std::shared_ptr<std::vector<uint8_t>>> bufferPool_;

  void decodeLoopFFmpeg(const std::string &url);
  bool tryOnceFFmpeg(const std::string &url);
  void cleanupFFmpeg();
};
