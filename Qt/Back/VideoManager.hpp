#pragma once

#include "CameraModel.hpp" // SlotInfo lives here
#include "VideoWorker.hpp"
#include <QMap>
#include <QObject>
#include <QStringList>

// VideoWorker 들의 영속 소유자.
// QML 에서 context property "videoManager" 로 접근한다.
//
// §8 VIDEO_STREAMING_SPEC changes:
//   - registerSlots(QList<SlotInfo>) replaces registerUrls()
//     (workers are keyed by URL internally; slots → worker resolved by slotId)
//   - getWorkerBySlot(int slotId) for tile-aware QML lookup
//   - registerUrls() kept for legacy signal path
class VideoManager : public QObject {
  Q_OBJECT

public:
  explicit VideoManager(QObject *parent = nullptr);
  ~VideoManager() override;

  // QML lookup by URL (legacy / VideoSurface.qml fallback)
  Q_INVOKABLE VideoWorker *getWorker(const QString &rtspUrl);

  // QML lookup by slotId — resolves slotId → rtspUrl → worker
  Q_INVOKABLE VideoWorker *getWorkerBySlot(int slotId);
  Q_INVOKABLE bool hasSlot(int slotId) const;
  Q_INVOKABLE QString slotUrl(int slotId) const;

  // Stop and destroy all workers (called on logout)
  Q_INVOKABLE void clearAll();

signals:
  void workerRegistered(const QString &rtspUrl);

public slots:
  // New path: receives SlotInfo list from CameraModel::slotsUpdated
  void registerSlots(const QList<SlotInfo> &slots);

  // Legacy: plain URL list from CameraModel::urlsUpdated
  void registerUrls(const QStringList &urls);

private:
  QMap<QString, VideoWorker *> m_workers; // URL → worker (owns)
  QMap<int, QString> m_slotToUrl;         // slotId → rtspUrl
};
