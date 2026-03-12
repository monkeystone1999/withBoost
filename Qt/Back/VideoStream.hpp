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

  QString rtspUrl() const { return rtspUrl_; }
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
    return frameSeq_.load(std::memory_order_acquire);
  }

signals:
  // Pull 기반 렌더링으로 전환됨.
  // 이 signal은 더 이상 emit되지 않지만, QML 호환성을 위해 선언을 유지.
  void frameReady();

private:
  QString rtspUrl_;
  std::unique_ptr<Video> video_;

  std::atomic<std::shared_ptr<std::vector<uint8_t>>> atomicBuffer_;
  std::atomic<int> atomicWidth_{0};
  std::atomic<int> atomicHeight_{0};
  std::atomic<int> atomicStride_{0};
  std::atomic<uint64_t> frameSeq_{0};
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
  QMap<QString, VideoWorker *> workers_;
  QMap<int, QString> slotToUrl_;
};
