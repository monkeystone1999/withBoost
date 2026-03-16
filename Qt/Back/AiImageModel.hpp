#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct AiImageEvent {
  QString deviceIp;
  QString deviceName;
  QString type;
  double confidence;
  long long timestamp;
  QString base64Image;
};

class AiImageModel : public QObject {
  Q_OBJECT
public:
  explicit AiImageModel(QObject *parent = nullptr);
  ~AiImageModel(); // Added destructor

  // QML Callable
  Q_INVOKABLE QVariantList getRecentEvents(const QString &ip) const;
  // QML-invokable method to get the latest event for a specific camera IP
  Q_INVOKABLE QVariantMap getLatestEvent(const QString &ip) const;

public slots:
  // Called by Network Bridge / Core
  void onImageReceived(const QString &ip, const QString &deviceName,
                       const QString &type, double confidence,
                       long long timestamp, const QByteArray &jpegData);

signals:
  void aiEventReceived(const QString &ip);

private:
  QList<AiImageEvent> events_; // Global rotating buffer (e.g. max 50)
};
