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
      std::function<void(std::shared_ptr<std::vector<uint8_t>>, int, int)>;

  explicit Video() = default;
  ~Video();

  // 스트리밍 제어
  void startStream(const std::string &url);
  void stopStream();

  // 프레임 콜백 등록 (VideoWorker 가 등록한다)
  FrameCallback onFrameReady;

private:
  // FFmpeg 컨텍스트
  AVFormatContext *m_formatCtx = nullptr;
  AVCodecContext *m_codecCtx = nullptr;
  struct SwsContext *m_swsCtx = nullptr;
  int m_videoStreamIndex = -1;

  // 디코딩 스레드
  std::thread m_decodeThread;
  std::atomic<bool> m_stopThread{false};

  // 버퍼 풀 (더블 버퍼 대신 여러 개의 버퍼를 풀링하여 재사용)
  std::vector<std::shared_ptr<std::vector<uint8_t>>> m_bufferPool;

  std::chrono::steady_clock::time_point m_lastUpdateTime;

  void decodeLoopFFmpeg(const std::string &url);
  void cleanupFFmpeg();
};
