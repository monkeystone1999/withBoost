#pragma once

#include "../../Src/Network/Video.hpp"
#include "Models/CameraModel.hpp"
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <atomic>
#include <functional>
#include <mutex>

class VideoWorker : public QObject {
  Q_OBJECT
public:
  explicit VideoWorker(const QString &cameraId, QObject *parent = nullptr);
  ~VideoWorker() override;

  void startStream();
  void stopStream();

  // Used by VideoManager to inject full connection string
  void setConnectionString(const QString &url) { connectionString_ = url; }
  void setFpsLimit(int fps) {
    fpsLimit_ = fps;
    if (video_)
      video_->setFpsLimit(fps);
  }

  uint64_t frameSeq() const {
    return frameSeq_.load(std::memory_order_acquire);
  }

  QVideoFrame getLatestFrame() {
    while (frameSpinLock_.test_and_set(std::memory_order_acquire)) {
    }
    QVideoFrame f = latestFrame_;
    frameSpinLock_.clear(std::memory_order_release);
    return f;
  }

signals:
  void frameReady(const QVideoFrame &frame);

private:
  QString cameraId_;
  QString connectionString_;
  std::unique_ptr<Video> video_;
  std::atomic<uint64_t> frameSeq_{0};
  std::atomic<bool> loggedFrameInfo_{false};
  int fpsLimit_ = 30;

  std::atomic_flag frameSpinLock_ = ATOMIC_FLAG_INIT;
  QVideoFrame latestFrame_;
};

class VideoManager : public QObject {
  Q_OBJECT
public:
  explicit VideoManager(QObject *parent = nullptr);
  ~VideoManager() override;

  Q_INVOKABLE VideoWorker *getWorker(const QString &cameraId);
  Q_INVOKABLE VideoWorker *getWorkerBySlot(int slotId);
  Q_INVOKABLE bool hasSlot(int slotId) const;
  Q_INVOKABLE QString slotCameraId(int slotId) const;

  void setUrlProvider(std::function<QString(const QString &)> provider) {
    urlProvider_ = std::move(provider);
  }
  void setFpsProvider(std::function<int()> provider) {
    fpsProvider_ = std::move(provider);
  }

public slots:
  void registerSlots(const QList<SlotInfo> &slots);
  void registerCameraIds(const QStringList &cameraIds);
  void restartWorker(const QString &cameraId);
  void setFpsLimit(int fps);
  void clearAll();

signals:
  void workerRegistered(const QString &cameraId);

private:
  QMap<QString, VideoWorker *> workers_;
  QMap<int, QString> slotToCameraId_;
  std::function<QString(const QString &)> urlProvider_;
  std::function<int()> fpsProvider_;
};

class VideoStream : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY
                 videoSinkChanged)
  Q_PROPERTY(int slotId READ slotId WRITE setSlotId NOTIFY slotIdChanged)
  Q_PROPERTY(
      QString cameraId READ cameraId WRITE setCameraId NOTIFY cameraIdChanged)

public:
  explicit VideoStream(QObject *parent = nullptr);
  ~VideoStream() override;

  QVideoSink *videoSink() const { return m_videoSink.data(); }
  void setVideoSink(QVideoSink *sink);

  int slotId() const { return m_slotId; }
  void setSlotId(int id);

  QString cameraId() const { return m_cameraId; }
  void setCameraId(const QString &id);

signals:
  void videoSinkChanged();
  void slotIdChanged();
  void cameraIdChanged();

private slots:
  void tryAttach();
  void onWorkerRegistered(const QString &id);

protected:
  void timerEvent(QTimerEvent *event) override;

private:
  QPointer<QVideoSink> m_videoSink;
  int m_slotId = -1;
  QString m_cameraId;
  QPointer<VideoWorker> m_worker;

  int m_timerId = 0;
  uint64_t m_lastSeq = 0;
};
