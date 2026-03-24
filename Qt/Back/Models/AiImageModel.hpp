#pragma once

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct AiImageEvent {
  QString cameraId; // "IP/streamNum" format
  QString deviceName;
  int trackId = 0;
  int frameIndex = 0;
  int totalFrames = 0;
  long long timestamp = 0;
  QString base64Image;
};

class AiImageModel : public QObject {
  Q_OBJECT
public:
  explicit AiImageModel(QObject *parent = nullptr);
  ~AiImageModel();

  // QML Callable — returns list of events for a specific camera
  Q_INVOKABLE QVariantList getEventsForCamera(const QString &cameraId) const;

  // QML Callable — returns the latest event for a specific camera
  Q_INVOKABLE QVariantMap getLatestEvent(const QString &cameraId) const;

  // Legacy: get events by IP (searches all cameraIds starting with IP)
  Q_INVOKABLE QVariantList getRecentEvents(const QString &ip) const;

  static constexpr int MAX_EVENTS_PER_CAMERA = 20;

public slots:
  // Called by Core — cameraId is already resolved (IP/streamNum format)
  void onImageReceivedBase64(const QString &cameraId, const QString &deviceName,
                             int trackId, int frameIndex, int totalFrames,
                             long long timestamp, const QString &base64Image);
  void clearAll();

signals:
  void aiEventReceived(const QString &cameraId);

private:
  QMap<QString, QList<AiImageEvent>> eventsByCameraId_;
};
