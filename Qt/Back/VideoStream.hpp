#pragma once

#include "../../Src/Network/Video.hpp"
#include "Camera.hpp"
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
#include <mutex>

class VideoWorker : public QObject {
  Q_OBJECT
public:
  explicit VideoWorker(const QString &rtspUrl, QObject *parent = nullptr);
  ~VideoWorker() override;

  void startStream();
  void stopStream();

  uint64_t frameSeq() const {
    return frameSeq_.load(std::memory_order_acquire);
  }

signals:
  void frameReady(const QVideoFrame &frame);

private:
  QString rtspUrl_;
  std::unique_ptr<Video> video_;
  std::atomic<uint64_t> frameSeq_{0};
  std::atomic<bool> loggedFrameInfo_{false};
};

class VideoManager : public QObject {
  Q_OBJECT
public:
  explicit VideoManager(QObject *parent = nullptr);
  ~VideoManager() override;

  Q_INVOKABLE VideoWorker *getWorker(const QString &rtspUrl);
  Q_INVOKABLE VideoWorker *getWorkerBySlot(int slotId);
  Q_INVOKABLE bool hasSlot(int slotId) const;
  Q_INVOKABLE QString slotUrl(int slotId) const;

public slots:
  void registerSlots(const QList<SlotInfo> &slots);
  void registerUrls(const QStringList &urls);
  void restartWorker(const QString &rtspUrl);
  void clearAll();

signals:
  void workerRegistered(const QString &rtspUrl);

private:
  QMap<QString, VideoWorker *> workers_;
  QMap<int, QString> slotToUrl_;
};

class VideoStream : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY
                 videoSinkChanged)
  Q_PROPERTY(int slotId READ slotId WRITE setSlotId NOTIFY slotIdChanged)
  Q_PROPERTY(
      QString rtspUrl READ rtspUrl WRITE setRtspUrl NOTIFY rtspUrlChanged)

public:
  explicit VideoStream(QObject *parent = nullptr);
  ~VideoStream() override;

  QVideoSink *videoSink() const { return m_videoSink; }
  void setVideoSink(QVideoSink *sink);

  int slotId() const { return m_slotId; }
  void setSlotId(int id);

  QString rtspUrl() const { return m_rtspUrl; }
  void setRtspUrl(const QString &url);

signals:
  void videoSinkChanged();
  void slotIdChanged();
  void rtspUrlChanged();

private slots:
  void tryAttach();
  void onWorkerRegistered(const QString &url);
  void handleNewFrame(const QVideoFrame &frame);

private:
  QVideoSink *m_videoSink = nullptr;
  int m_slotId = -1;
  QString m_rtspUrl;
  QPointer<VideoWorker> m_worker;
};
