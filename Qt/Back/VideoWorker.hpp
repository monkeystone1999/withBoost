#pragma once

#include <QObject>
#include <QString>
#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

#include "Network/Video.hpp"

class VideoWorker : public QObject {
  Q_OBJECT

public:
  explicit VideoWorker(const QString &rtspUrl, QObject *parent = nullptr);
  ~VideoWorker() override;

  QString rtspUrl() const { return m_rtspUrl; }
  void startStream();
  void stopStream();

  struct FrameData {
    std::shared_ptr<std::vector<uint8_t>> buffer;
    int width = 0;
    int height = 0;
    int stride = 0;
  };

  // 렌더 스레드에서 호출 — lock 없이 최신 프레임을 가져간다
  FrameData getLatestFrame() const;

signals:
  void frameReady();

private:
  QString m_rtspUrl;
  std::unique_ptr<Video> m_video;

  // FFmpeg 스레드(쓰기) ↔ 렌더 스레드(읽기) — lock-free atomic
  std::atomic<std::shared_ptr<std::vector<uint8_t>>> m_atomicBuffer;
  std::atomic<int> m_atomicWidth{0};
  std::atomic<int> m_atomicHeight{0};
  std::atomic<int> m_atomicStride{0};

  // Coalescing 플래그
  std::atomic<bool> m_signalPending{false};
  std::chrono::steady_clock::time_point m_lastFrameTime;
};
