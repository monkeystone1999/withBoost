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

/**
 * @section Workflow Guide
 *
 * **[AiImageModel 구현 상세 내역]**
 *
 * 1. 큐 운영 전략 (Prepend vs Append):
 *    - 내부적으로 `Append` 방식으로 데이터를 추가하고, `removeFirst`를 통해
 * 오래된 데이터를 제거하는 FIFO(First-In-First-Out) 방식을 준수합니다.
 *
 * 2. 카메라 식별 규칙:
 *    - `cameraId`는 단순히 IP 주소가 아니라 `IP/StreamIndex` 형태를 띱니다.
 *    - `getRecentEvents(ip)`는 이 규칙에 따라 특정 장치의 모든 스트림 이벤트를
 * 조회할 수 있도록 `startsWith` 검색 기능을 제공합니다.
 *
 * 3. 메모리 효율성:
 *    - Base64 데이터는 메모리 점유율이 높으므로 `MAX_EVENTS_PER_CAMERA`를 통해
 * 런타임 가용 메모리를 엄격히 통제합니다.
 */
