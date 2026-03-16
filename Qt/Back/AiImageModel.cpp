#include "AiImageModel.hpp"
#include <QDebug>

AiImageModel::AiImageModel(QObject *parent) : QObject(parent) {}

AiImageModel::~AiImageModel() {}

void AiImageModel::onImageReceived(const QString &ip, const QString &deviceName,
                                   const QString &type, double confidence,
                                   long long timestamp,
                                   const QByteArray &jpegData) {
  AiImageEvent ev;
  ev.deviceIp = ip;
  ev.deviceName = deviceName;
  ev.type = type;
  ev.confidence = confidence;
  ev.timestamp = timestamp;

  // QML can display base64 easily via <Image
  // source="data:image/jpeg;base64,..." />
  ev.base64Image =
      "data:image/jpeg;base64," + QString::fromLatin1(jpegData.toBase64());

  events_.append(ev);
  if (events_.size() > 50) {
    events_.removeFirst();
  }

  qDebug() << "[AiImageModel] Received Event for" << ip << type << confidence;

  emit aiEventReceived(ip);
}

QVariantList AiImageModel::getRecentEvents(const QString &ip) const {
  QVariantList list;
  for (const auto &ev : events_) {
    if (ev.deviceIp == ip) {
      QVariantMap m;
      m["deviceIp"] = ev.deviceIp;
      m["deviceName"] = ev.deviceName;
      m["type"] = ev.type;
      m["confidence"] = ev.confidence;
      m["timestamp"] = ev.timestamp;
      m["base64Image"] = ev.base64Image;
      list.append(m);
    }
  }
  return list;
}

QVariantMap AiImageModel::getLatestEvent(const QString &ip) const {
  for (int i = events_.size() - 1; i >= 0; --i) {
    if (events_[i].deviceIp == ip) {
      QVariantMap m;
      m["deviceIp"] = events_[i].deviceIp;
      m["deviceName"] = events_[i].deviceName;
      m["type"] = events_[i].type;
      m["confidence"] = events_[i].confidence;
      m["timestamp"] = events_[i].timestamp;
      m["base64Image"] = events_[i].base64Image;
      return m;
    }
  }
  return {};
}
