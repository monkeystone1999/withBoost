#include "VideoWorker.hpp"
#include <QMetaObject>

VideoWorker::VideoWorker(const QString &rtspUrl, QObject *parent)
    : QObject(parent), m_rtspUrl(rtspUrl), m_video(std::make_unique<Video>()),
      m_lastFrameTime(std::chrono::steady_clock::now()) {

  m_video->onFrameReady = [this](std::shared_ptr<std::vector<uint8_t>> buf,
                                 int w, int h, int stride) {
    // ── 30fps 제한 (FFmpeg 스레드) ───────────────────────────────────────────
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_lastFrameTime)
                    .count();
    if (diff < 33)
      return;
    m_lastFrameTime = now;

    // ── 핵심: 버퍼를 atomic 으로 교체 (FFmpeg 스레드에서, lock-free) ─────────
    // GUI 스레드는 이 데이터에 전혀 관여하지 않는다
    // 렌더 스레드가 updatePaintNode() 에서 직접 읽어간다
    m_atomicWidth.store(w, std::memory_order_relaxed);
    m_atomicHeight.store(h, std::memory_order_relaxed);
    m_atomicStride.store(stride, std::memory_order_relaxed);
    m_atomicBuffer.store(std::move(buf), std::memory_order_release);

    // ── Coalescing: GUI 큐에 이벤트가 이미 있으면 추가 등록 안 함 ───────────
    if (m_signalPending.exchange(true, std::memory_order_acq_rel))
      return;

    // ── GUI 스레드에 "렌더 예약"만 전달 — 데이터 이동 없음 ──────────────────
    // invokeMethod 람다에서 하는 일: update() 호출 하나뿐
    // GUI 스레드가 frameData 를 복사하거나 이동하는 코드가 완전히 사라짐
    QMetaObject::invokeMethod(
        this,
        [this]() {
          m_signalPending.store(false, std::memory_order_release);
          emit frameReady();
        },
        Qt::QueuedConnection);
  };
}

VideoWorker::~VideoWorker() { stopStream(); }

void VideoWorker::startStream() {
  m_video->startStream(m_rtspUrl.toStdString());
}
void VideoWorker::stopStream() { m_video->stopStream(); }

// 렌더 스레드에서 호출 — atomic load, 뮤텍스 없음
VideoWorker::FrameData VideoWorker::getLatestFrame() const {
  FrameData f;
  f.buffer = m_atomicBuffer.load(std::memory_order_acquire);
  f.width = m_atomicWidth.load(std::memory_order_relaxed);
  f.height = m_atomicHeight.load(std::memory_order_relaxed);
  f.stride = m_atomicStride.load(std::memory_order_relaxed);
  return f;
}
