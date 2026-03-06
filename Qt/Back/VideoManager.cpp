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
  if (it != m_workers.end())
    return it.value();
  return nullptr;
}

void VideoManager::clearAll() {
  for (auto *worker : m_workers) {
    worker->stopStream();
    delete worker;
  }
  m_workers.clear();
}

void VideoManager::registerUrls(const QStringList &urls) {
  for (const QString &url : urls) {
    if (url.isEmpty())
      continue;
    if (m_workers.contains(url))
      continue; // 이미 존재 → 아무것도 안 함

    qDebug() << "[VideoManager] 신규 스트림 등록:" << url;
    auto *worker = new VideoWorker(url, this);
    m_workers.insert(url, worker);
    worker->startStream();
    emit workerRegistered(url);
  }
}
