#pragma once

#include "../../Src/Network/Video.hpp"
#include "Camera.hpp"
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <atomic>
#include <memory>

class VideoWorker : public QObject {
  Q_OBJECT
public:
  struct FrameData {
    std::shared_ptr<QByteArray> buffer;
    int width = 0;
    int height = 0;
    int strideY = 0;
    int strideUV = 0;
    int offsetUV = 0;
    int format = 0;
  };

  explicit VideoWorker(const QString &rtspUrl, QObject *parent = nullptr);
  ~VideoWorker() override;

  void startStream();
  void stopStream();

  FrameData getLatestFrame() const;

  uint64_t frameSeq() const {
    return frameSeq_.load(std::memory_order_acquire);
  }

signals:
  void frameReady();

private:
  QString rtspUrl_;
  std::unique_ptr<Video> video_;
  QByteArray workingBuffer_;

  std::atomic<std::shared_ptr<QByteArray>> atomicBuffer_;
  std::atomic<int> atomicWidth_{0};
  std::atomic<int> atomicHeight_{0};
  std::atomic<int> atomicStrideY_{0};
  std::atomic<int> atomicStrideUV_{0};
  std::atomic<int> atomicOffsetUV_{0};
  std::atomic<int> atomicFormat_{0}; // 0 = NV12, 1 = RGBA
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
