#include "VideoStream.hpp"
#include <QDebug>
#include <QMetaObject>


// ==============================================================================
// VideoWorker Implementation
// ==============================================================================

VideoWorker::VideoWorker(const QString &rtspUrl, QObject *parent)
    : QObject(parent), m_rtspUrl(rtspUrl), m_video(std::make_unique<Video>()) {

  // ──────────────────────────────────────────────────────────────────────────
  // onFrameReady 콜백: Video 클래스로부터 디코딩된 프레임 수신
  //
  // 이전: 33ms 제한으로 약 30fps로 throttling
  // 현재: 모든 프레임을 즉시 atomic 버퍼에 저장 (실시간 스트리밍 최적화)
  //
  // 동작 원리:
  // 1. Video::onFrameReady가 디코딩된 모든 프레임을 즉시 전달
  // 2. atomic store로 lock-free 덮어쓰기 (중간 프레임 자동 스킵)
  // 3. frameSeq 증가로 VideoSurfaceItem에 새 프레임 통지
  // 4. VideoSurfaceItem의 beforeSynchronizing이 렌더링 주기에 맞춰
  //    최신 프레임만 atomic read (GUI Thread Sync 영향 없음)
  //
  // 성능 영향:
  // - atomic store: ~수십 나노초 (매우 빠름)
  // - GUI Thread Sync: 영향 없음 (atomic read도 ~수십 나노초)
  // - 메모리: shared_ptr 덮어쓰기로 증가 없음
  // - 실시간성: FFmpeg 디코딩 지연 최소화
  // ──────────────────────────────────────────────────────────────────────────
  m_video->onFrameReady = [this](std::shared_ptr<std::vector<uint8_t>> buf,
                                 int w, int h, int stride) {
    m_atomicWidth.store(w, std::memory_order_relaxed);
    m_atomicHeight.store(h, std::memory_order_relaxed);
    m_atomicStride.store(stride, std::memory_order_relaxed);
    m_atomicBuffer.store(std::move(buf), std::memory_order_relaxed);
    m_frameSeq.fetch_add(1, std::memory_order_release);
    
    // ────────────────────────────────────────────────────────────────────────
    // CRITICAL: 실시간 렌더링 트리거
    //
    // 새 프레임이 도착했음을 VideoSurfaceItem에 통지하여
    // update()를 호출하게 함으로써 화면 갱신을 트리거.
    //
    // Qt::QueuedConnection으로 자동 전달되므로 스레드 안전.
    // (Video 스레드에서 emit → GUI 스레드에서 수신)
    // ────────────────────────────────────────────────────────────────────────
    emit frameReady();
  };
}

VideoWorker::~VideoWorker() { stopStream(); }

void VideoWorker::startStream() {
  m_video->startStream(m_rtspUrl.toStdString());
}
void VideoWorker::stopStream() { m_video->stopStream(); }

VideoWorker::FrameData VideoWorker::getLatestFrame() const {
  FrameData f;
  f.buffer = m_atomicBuffer.load(std::memory_order_acquire);
  f.width = m_atomicWidth.load(std::memory_order_relaxed);
  f.height = m_atomicHeight.load(std::memory_order_relaxed);
  f.stride = m_atomicStride.load(std::memory_order_relaxed);
  return f;
}

// ==============================================================================
// VideoManager Implementation
// ==============================================================================

VideoManager::VideoManager(QObject *parent) : QObject(parent) {}

VideoManager::~VideoManager() {
  for (auto *worker : m_workers) {
    worker->stopStream();
    delete worker;
  }
  m_workers.clear();
}

VideoWorker *VideoManager::getWorker(const QString &rtspUrl) {
  auto it = m_workers.find(rtspUrl);
  return it != m_workers.end() ? it.value() : nullptr;
}

VideoWorker *VideoManager::getWorkerBySlot(int slotId) {
  auto it = m_slotToUrl.find(slotId);
  if (it == m_slotToUrl.end())
    return nullptr;
  return getWorker(it.value());
}

bool VideoManager::hasSlot(int slotId) const {
  return m_slotToUrl.contains(slotId);
}

QString VideoManager::slotUrl(int slotId) const {
  auto it = m_slotToUrl.find(slotId);
  return it != m_slotToUrl.end() ? it.value() : QString{};
}

void VideoManager::clearAll() {
  for (auto *worker : m_workers) {
    worker->stopStream();
    delete worker;
  }
  m_workers.clear();
  m_slotToUrl.clear();
}

void VideoManager::registerSlots(const QList<SlotInfo> &Slots) {
  m_slotToUrl.clear();
  for (const SlotInfo &si : Slots)
    m_slotToUrl.insert(si.slotId, si.rtspUrl);

  QStringList uniqueUrls;
  for (const SlotInfo &si : Slots)
    if (!uniqueUrls.contains(si.rtspUrl))
      uniqueUrls << si.rtspUrl;

  for (const QString &url : uniqueUrls) {
    if (url.isEmpty() || m_workers.contains(url))
      continue;
    qDebug() << "[VideoManager] 신규 스트림 등록 (slot):" << url;
    auto *worker = new VideoWorker(url, this);
    m_workers.insert(url, worker);
    worker->startStream();
    emit workerRegistered(url);
  }
}

void VideoManager::registerUrls(const QStringList &urls) {
  for (const QString &url : urls) {
    if (url.isEmpty() || m_workers.contains(url))
      continue;
    qDebug() << "[VideoManager] " << url;
    auto *worker = new VideoWorker(url, this);
    m_workers.insert(url, worker);
    worker->startStream();
    emit workerRegistered(url);
  }
}

void VideoManager::restartWorker(const QString &rtspUrl) {
  auto it = m_workers.find(rtspUrl);
  if (it == m_workers.end()) {
    if (rtspUrl.isEmpty())
      return;
    qDebug() << "[VideoManager] restartWorker: 새 워커 생성" << rtspUrl;
    auto *worker = new VideoWorker(rtspUrl, this);
    m_workers.insert(rtspUrl, worker);
    worker->startStream();
    emit workerRegistered(rtspUrl);
    return;
  }
  
  qDebug() << "[VideoManager] restartWorker: 재시작" << rtspUrl;
  VideoWorker *worker = it.value();
  worker->stopStream();
  worker->startStream();
  
  // ────────────────────────────────────────────────────────────────────────
  // CRITICAL: VideoSurface 재연결 트리거
  // 
  // 이미 연결된 VideoSurface가 있더라도 workerRegistered를 발신해야 함.
  // VideoSurface._onRegistered()는 동일 worker 포인터를 다시 설정하므로
  // 렌더링 중단 없이 안전하게 재연결됨.
  // ────────────────────────────────────────────────────────────────────────
  emit workerRegistered(rtspUrl);
}
