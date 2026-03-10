#include "VideoManager.hpp"
#include <QDebug>

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

// §3: slot-based lookup — resolves slotId → rtspUrl → worker
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

// §3: new primary registration path — receives SlotInfo list from CameraModel
void VideoManager::registerSlots(const QList<SlotInfo> &Slots) {
  // Rebuild slot→URL map
  m_slotToUrl.clear();
  for (const SlotInfo &si : Slots)
    m_slotToUrl.insert(si.slotId, si.rtspUrl);

  // Collect unique URLs and start workers for new ones
  QStringList uniqueUrls;
  for (const SlotInfo &si : Slots)
    if (!uniqueUrls.contains(si.rtspUrl))
      uniqueUrls << si.rtspUrl;

  // Do not stop/remove existing workers during live slot updates.
  // CameraModel can reorder/split/merge rapidly; keeping workers alive avoids
  // transient teardown/restart glitches in streaming surfaces.

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

// Legacy: plain URL list (still works — connected from urlsUpdated signal)
void VideoManager::registerUrls(const QStringList &urls) {
  for (const QString &url : urls) {
    if (url.isEmpty() || m_workers.contains(url))
      continue;
    qDebug() << "[VideoManager] 신규 스트림 등록:" << url;
    auto *worker = new VideoWorker(url, this);
    m_workers.insert(url, worker);
    worker->startStream();
    emit workerRegistered(url);
  }
}

// F-2: restart an existing worker whose camera came back online
void VideoManager::restartWorker(const QString &rtspUrl) {
  auto it = m_workers.find(rtspUrl);
  if (it == m_workers.end()) {
    // Worker didn't exist yet — register it fresh
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
  it.value()->stopStream();
  it.value()->startStream();
  emit workerRegistered(rtspUrl); // VideoSurface가 재연결 인지
}
