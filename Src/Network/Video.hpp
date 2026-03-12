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

// Pure C++ FFmpeg 디코더.
// Qt에 전혀 의존하지 않는다.
// 프레임이 디코딩될 때마다 onFrameReady 콜백을 호출한다.
class Video {
public:
  // 프레임 콜백: RGB24 데이터 버퍼 (shared_ptr로 소유권 공유), 너비, 높이
  using FrameCallback =
      std::function<void(std::shared_ptr<std::vector<uint8_t>>, int, int, int)>;

  explicit Video() = default;
  ~Video();

  // 스트리밍 제어
  void startStream(const std::string &url);
  void stopStream();

  // 프레임 콜백 등록 (VideoWorker 가 등록한다)
  FrameCallback onFrameReady;

private:
  // FFmpeg 컨텍스트
  AVFormatContext *formatCtx_ = nullptr;
  AVCodecContext *codecCtx_ = nullptr;
  struct SwsContext *swsCtx_ = nullptr;
  AVBufferRef* hw_device_ctx = nullptr;
  int videoStreamIndex_ = -1;

  // Decode thread
  std::thread decodeThread_;
  std::atomic<bool> stopThread_{false};

  // 버퍼 풀 (더블 버퍼 대신 여러 개의 버퍼를 풀링하여 재사용)
  std::vector<std::shared_ptr<std::vector<uint8_t>>> bufferPool_;

  void decodeLoopFFmpeg(const std::string &url);
  bool tryOnceFFmpeg(const std::string &url);
  void cleanupFFmpeg();
};
