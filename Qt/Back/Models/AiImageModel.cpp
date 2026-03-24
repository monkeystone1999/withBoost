#include "AiImageModel.hpp"
#include <QDebug>

AiImageModel::AiImageModel(QObject *parent) : QObject(parent) {}

AiImageModel::~AiImageModel() {}

void AiImageModel::onImageReceivedBase64(const QString &cameraId,
                                         const QString &deviceName, int trackId,
                                         int frameIndex, int totalFrames,
                                         long long timestamp,
                                         const QString &base64Image) {
  AiImageEvent ev;
  ev.cameraId = cameraId;
  ev.deviceName = deviceName;
  ev.trackId = trackId;
  ev.frameIndex = frameIndex;
  ev.totalFrames = totalFrames;
  ev.timestamp = timestamp;
  ev.base64Image = base64Image;

  auto &queue = eventsByCameraId_[cameraId];
  queue.append(ev);
  while (queue.size() > MAX_EVENTS_PER_CAMERA) {
    queue.removeFirst();
  }

  emit aiEventReceived(cameraId);
}

QVariantList AiImageModel::getEventsForCamera(const QString &cameraId) const {
  QVariantList list;
  auto it = eventsByCameraId_.find(cameraId);
  if (it == eventsByCameraId_.end())
    return list;

  for (const auto &ev : it.value()) {
    QVariantMap m;
    m["cameraId"] = ev.cameraId;
    m["deviceName"] = ev.deviceName;
    m["trackId"] = ev.trackId;
    m["frameIndex"] = ev.frameIndex;
    m["totalFrames"] = ev.totalFrames;
    m["timestamp"] = ev.timestamp;
    m["base64Image"] = ev.base64Image;
    list.append(m);
  }
  return list;
}

QVariantMap AiImageModel::getLatestEvent(const QString &cameraId) const {
  auto it = eventsByCameraId_.find(cameraId);
  if (it == eventsByCameraId_.end() || it.value().isEmpty())
    return {};

  const auto &ev = it.value().last();
  QVariantMap m;
  m["cameraId"] = ev.cameraId;
  m["deviceName"] = ev.deviceName;
  m["trackId"] = ev.trackId;
  m["frameIndex"] = ev.frameIndex;
  m["totalFrames"] = ev.totalFrames;
  m["timestamp"] = ev.timestamp;
  m["base64Image"] = ev.base64Image;
  return m;
}

QVariantList AiImageModel::getRecentEvents(const QString &ip) const {
  QVariantList list;
  for (auto it = eventsByCameraId_.begin(); it != eventsByCameraId_.end();
       ++it) {
    if (it.key().startsWith(ip)) {
      for (const auto &ev : it.value()) {
        QVariantMap m;
        m["cameraId"] = ev.cameraId;
        m["deviceName"] = ev.deviceName;
        m["trackId"] = ev.trackId;
        m["frameIndex"] = ev.frameIndex;
        m["totalFrames"] = ev.totalFrames;
        m["timestamp"] = ev.timestamp;
        m["base64Image"] = ev.base64Image;
        list.append(m);
      }
    }
  }
  return list;
}

void AiImageModel::clearAll() { eventsByCameraId_.clear(); }
