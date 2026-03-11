#pragma once
#include "Camera.hpp"
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>
#include <chrono>
#include <memory>
#include <vector>


#include "Network/Video.hpp"

class VideoWorker : public QObject {
  Q_OBJECT

public:
  explicit VideoWorker(const QString &rtspUrl, QObject *parent = nullptr);
  ~VideoWorker() override;

  QString rtspUrl() const { return m_rtspUrl; }
  void startStream();
  void stopStream();

  struct FrameData {
    std::shared_ptr<std::vector<uint8_t>> buffer;
    int width = 0;
    int height = 0;
    int stride = 0;
  };

  FrameData getLatestFrame() const;
  uint64_t frameSeq() const {
    return m_frameSeq.load(std::memory_order_acquire);
  }

signals:
  void frameReady();

private:
  QString m_rtspUrl;
  std::unique_ptr<Video> m_video;

  std::atomic<std::shared_ptr<std::vector<uint8_t>>> m_atomicBuffer;
  std::atomic<int> m_atomicWidth{0};
  std::atomic<int> m_atomicHeight{0};
  std::atomic<int> m_atomicStride{0};
  std::atomic<uint64_t> m_frameSeq{0};
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
  Q_INVOKABLE void clearAll();

signals:
  void workerRegistered(const QString &rtspUrl);

public slots:
  void registerSlots(const QList<SlotInfo> &slots);
  void registerUrls(const QStringList &urls);
  void restartWorker(const QString &rtspUrl);

private:
  QMap<QString, VideoWorker *> m_workers;
  QMap<int, QString> m_slotToUrl;
};
