#include "VideoStream.hpp"
#include <QDebug>
#include <QMetaObject>

// ==============================================================================
// VideoWorker Implementation
// ==============================================================================

VideoWorker::VideoWorker(const QString &rtspUrl, QObject *parent)
    : QObject(parent), rtspUrl_(rtspUrl), video_(std::make_unique<Video>()) {

  // ──────────────────────────────────────────────────────────────────────────
  // Pull 기반 렌더링 최적화
  //
  // 변경점: emit frameReady() 제거
  //
  // 이전: 매 프레임마다 frameReady() signal 발신 → GUI 스레드에서 update() 호출
  //       → Signal/Slot 오버헤드 + 불규칙한 타이밍 → UI stutter 유발
  //
  // 현재: beforeSynchronizing에서 렌더링 주기(~16.67ms for 60Hz)에 맞춰
  //       최신 프레임을 직접 pull. Signal/Slot 오버헤드 완전 제거.
  //
  // 동작 원리:
  // 1. Video::onFrameReady가 디코딩된 모든 프레임을 즉시 전달
  // 2. atomic store로 lock-free 덮어쓰기 (중간 프레임 자동 스킵)
  // 3. frameSeq 증가로 VideoSurfaceItem에 새 프레임 존재 통지
  // 4. VideoSurfaceItem의 beforeSynchronizing이 렌더링 주기에 맞춰
  //    최신 프레임만 atomic read (GUI Thread Sync 영향 없음)
  // ──────────────────────────────────────────────────────────────────────────
  video_->onFrameReady = [this](std::shared_ptr<std::vector<uint8_t>> buf,
                                 int w, int h, int stride) {
    atomicWidth_.store(w, std::memory_order_relaxed);
    atomicHeight_.store(h, std::memory_order_relaxed);
    atomicStride_.store(stride, std::memory_order_relaxed);
    atomicBuffer_.store(std::move(buf), std::memory_order_relaxed);
    frameSeq_.fetch_add(1, std::memory_order_release);

    // ────────────────────────────────────────────────────────────────────────
    // ✅ Pull 기반 렌더링: emit frameReady() 제거!
    //
    // beforeSynchronizing에서 렌더링 주기에 정확히 동기화하여 프레임 획득.
    // Signal/Slot 오버헤드 제거 + UI stutter 완전 제거.
    // ────────────────────────────────────────────────────────────────────────
  };
}

VideoWorker::~VideoWorker() { stopStream(); }

void VideoWorker::startStream() {
  video_->startStream(rtspUrl_.toStdString());
}
void VideoWorker::stopStream() { video_->stopStream(); }

VideoWorker::FrameData VideoWorker::getLatestFrame() const {
  FrameData f;
  f.buffer = atomicBuffer_.load(std::memory_order_acquire);
  f.width = atomicWidth_.load(std::memory_order_relaxed);
  f.height = atomicHeight_.load(std::memory_order_relaxed);
  f.stride = atomicStride_.load(std::memory_order_relaxed);
  return f;
}

// ==============================================================================
// VideoManager Implementation
// ==============================================================================

VideoManager::VideoManager(QObject *parent) : QObject(parent) {}

VideoManager::~VideoManager() {
  for (auto *worker : workers_) {
    worker->stopStream();
    delete worker;
  }
  workers_.clear();
}

VideoWorker *VideoManager::getWorker(const QString &rtspUrl) {
  auto it = workers_.find(rtspUrl);
  return it != workers_.end() ? it.value() : nullptr;
}

VideoWorker *VideoManager::getWorkerBySlot(int slotId) {
  auto it = slotToUrl_.find(slotId);
  if (it == slotToUrl_.end())
    return nullptr;
  return getWorker(it.value());
}

bool VideoManager::hasSlot(int slotId) const {
  return slotToUrl_.contains(slotId);
}

QString VideoManager::slotUrl(int slotId) const {
  auto it = slotToUrl_.find(slotId);
  return it != slotToUrl_.end() ? it.value() : QString{};
}

void VideoManager::clearAll() {
  for (auto *worker : workers_) {
    worker->stopStream();
    delete worker;
  }
  workers_.clear();
  slotToUrl_.clear();
}

void VideoManager::registerSlots(const QList<SlotInfo> &Slots) {
  slotToUrl_.clear();
  for (const SlotInfo &si : Slots)
    slotToUrl_.insert(si.slotId, si.rtspUrl);

  QStringList uniqueUrls;
  for (const SlotInfo &si : Slots)
    if (!uniqueUrls.contains(si.rtspUrl))
      uniqueUrls << si.rtspUrl;

  for (const QString &url : uniqueUrls) {
    if (url.isEmpty() || workers_.contains(url))
      continue;
    qDebug() << "[VideoManager] 신규 스트림 등록 (slot):" << url;
    auto *worker = new VideoWorker(url, this);
    workers_.insert(url, worker);
    worker->startStream();
    emit workerRegistered(url);
  }
}

void VideoManager::registerUrls(const QStringList &urls) {
  for (const QString &url : urls) {
    if (url.isEmpty() || workers_.contains(url))
      continue;
    qDebug() << "[VideoManager] " << url;
    auto *worker = new VideoWorker(url, this);
    workers_.insert(url, worker);
    worker->startStream();
    emit workerRegistered(url);
  }
}

void VideoManager::restartWorker(const QString &rtspUrl) {
  auto it = workers_.find(rtspUrl);
  if (it == workers_.end()) {
    if (rtspUrl.isEmpty())
      return;
    qDebug() << "[VideoManager] restartWorker: 새 워커 생성" << rtspUrl;
    auto *worker = new VideoWorker(rtspUrl, this);
    workers_.insert(rtspUrl, worker);
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
