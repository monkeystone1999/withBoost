#include "VideoStream.hpp"
#include <QDebug>
#include <QMetaObject>
#include <libyuv.h>

VideoWorker::VideoWorker(const QString &rtspUrl, QObject *parent)
    : QObject(parent), rtspUrl_(rtspUrl), video_(std::make_unique<Video>()) {

  video_->onFrameReady = [this](const Video::FramePayload &payload) {
    if (!payload.buffer || payload.buffer->empty() || payload.width <= 0 ||
        payload.height <= 0)
      return;

    int w = payload.width;
    int h = payload.height;
    int uvOffset =
        payload.offsetUV > 0 ? payload.offsetUV : payload.strideY * h;
    int rgbaStride = w * 4;

    auto rgbaBuf = std::make_shared<std::vector<uint8_t>>(rgbaStride * h);
    const uint8_t *srcY = payload.buffer->data();
    const uint8_t *srcUV = payload.buffer->data() + uvOffset;
    uint8_t *dstRGBA = rgbaBuf->data();

    libyuv::NV12ToABGR(srcY, payload.strideY, srcUV, payload.strideUV, dstRGBA,
                       rgbaStride, w, h);

    atomicBuffer_.store(rgbaBuf, std::memory_order_release);
    atomicWidth_.store(w, std::memory_order_relaxed);
    atomicHeight_.store(h, std::memory_order_relaxed);
    atomicStrideY_.store(rgbaStride, std::memory_order_relaxed);
    atomicStrideUV_.store(0, std::memory_order_relaxed);
    atomicOffsetUV_.store(0, std::memory_order_relaxed);
    atomicFormat_.store(1, std::memory_order_relaxed); // 1 = RGBA

    frameSeq_.fetch_add(1, std::memory_order_release);

    if (!loggedFrameInfo_.load()) {
      loggedFrameInfo_.store(true);
      qWarning() << "[VideoWorker] libyuv converted frame w=" << w << "h=" << h
                 << "rgba_stride=" << rgbaStride;
    }

    emit frameReady();
  };
}

VideoWorker::~VideoWorker() { stopStream(); }

void VideoWorker::startStream() { video_->startStream(rtspUrl_.toStdString()); }
void VideoWorker::stopStream() { video_->stopStream(); }

VideoWorker::FrameData VideoWorker::getLatestFrame() const {
  FrameData f;
  f.buffer = atomicBuffer_.load(std::memory_order_acquire);
  f.width = atomicWidth_.load(std::memory_order_relaxed);
  f.height = atomicHeight_.load(std::memory_order_relaxed);
  f.strideY = atomicStrideY_.load(std::memory_order_relaxed);
  f.strideUV = atomicStrideUV_.load(std::memory_order_relaxed);
  f.offsetUV = atomicOffsetUV_.load(std::memory_order_relaxed);
  f.format = atomicFormat_.load(std::memory_order_relaxed);

  if (!f.buffer) {
    qDebug() << "[VideoWorker] getLatestFrame() called but buffer is null.";
  }
  return f;
}

VideoManager::VideoManager(QObject *parent) : QObject(parent) {}

VideoManager::~VideoManager() { clearAll(); }

VideoWorker *VideoManager::getWorker(const QString &rtspUrl) {
  return workers_.value(rtspUrl, nullptr);
}

VideoWorker *VideoManager::getWorkerBySlot(int slotId) {
  return getWorker(slotToUrl_.value(slotId));
}

bool VideoManager::hasSlot(int slotId) const {
  return slotToUrl_.contains(slotId);
}
QString VideoManager::slotUrl(int slotId) const {
  return slotToUrl_.value(slotId);
}

void VideoManager::clearAll() {
  for (auto *worker : workers_) {
    worker->stopStream();
    delete worker;
  }
  workers_.clear();
  slotToUrl_.clear();
}

void VideoManager::registerSlots(const QList<SlotInfo> &Slots) {
  slotToUrl_.clear();
  for (const SlotInfo &si : Slots)
    slotToUrl_.insert(si.slotId, si.rtspUrl);

  for (const SlotInfo &si : Slots) {
    if (si.rtspUrl.isEmpty() || workers_.contains(si.rtspUrl))
      continue;
    auto *worker = new VideoWorker(si.rtspUrl, this);
    workers_.insert(si.rtspUrl, worker);
    worker->startStream();
    emit workerRegistered(si.rtspUrl);
  }
}

void VideoManager::registerUrls(const QStringList &urls) {
  for (const QString &url : urls) {
    if (url.isEmpty() || workers_.contains(url))
      continue;
    auto *worker = new VideoWorker(url, this);
    workers_.insert(url, worker);
    worker->startStream();
    emit workerRegistered(url);
  }
}

void VideoManager::restartWorker(const QString &rtspUrl) {
  auto *worker = workers_.value(rtspUrl);
  if (!worker) {
    if (rtspUrl.isEmpty())
      return;
    worker = new VideoWorker(rtspUrl, this);
    workers_.insert(rtspUrl, worker);
    worker->startStream();
    emit workerRegistered(rtspUrl);
  } else {
    worker->stopStream();
    worker->startStream();
    emit workerRegistered(rtspUrl);
  }
}
