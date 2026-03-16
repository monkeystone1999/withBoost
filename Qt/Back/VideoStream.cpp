#include "VideoStream.hpp"
#include <QDebug>
#include <QMetaObject>

VideoWorker::VideoWorker(const QString &rtspUrl, QObject *parent)
    : QObject(parent), rtspUrl_(rtspUrl), video_(std::make_unique<Video>()) {

  video_->onFrameReady = [this](const Video::FramePayload &payload) {
    if (!payload.dataY || !payload.dataUV || payload.width <= 0 ||
        payload.height <= 0)
      return;

    int w = payload.width;
    int h = payload.height;

    QVideoFrameFormat format(QSize(w, h), QVideoFrameFormat::Format_NV12);
    QVideoFrame frame(format);

    if (frame.map(QVideoFrame::WriteOnly)) {
      int y_size = w * h;
      int uv_size = w * h / 2;

      // Copy Y plane
      // Since NV12 Y-stride == width, we can use simple memcpy if stride
      // matches exactly, but if padding exists, we should loop.
      if (payload.strideY == w) {
        std::memcpy(frame.bits(0), payload.dataY, y_size);
      } else {
        for (int i = 0; i < h; ++i) {
          std::memcpy(frame.bits(0) + (i * w),
                      payload.dataY + (i * payload.strideY), w);
        }
      }

      // Copy UV plane
      if (payload.strideUV == w) {
        std::memcpy(frame.bits(1), payload.dataUV, uv_size);
      } else {
        for (int i = 0; i < h / 2; ++i) {
          std::memcpy(frame.bits(1) + (i * w),
                      payload.dataUV + (i * payload.strideUV), w);
        }
      }

      frame.unmap();
      frameSeq_.fetch_add(1, std::memory_order_release);

      if (!loggedFrameInfo_.load()) {
        loggedFrameInfo_.store(true);
        qWarning() << "[VideoWorker] NV12 mapped frame w=" << w << "h=" << h;
      }

      emit frameReady(frame);
    }
  };
}

VideoWorker::~VideoWorker() { stopStream(); }

void VideoWorker::startStream() { video_->startStream(rtspUrl_.toStdString()); }
void VideoWorker::stopStream() { video_->stopStream(); }

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

// ────────────────────────────────────────────────────────────────────────
// VideoStream (QML <-> VideoWorker Bridge)
// ────────────────────────────────────────────────────────────────────────

extern VideoManager *videoManager; // Required to fetch the worker

VideoStream::VideoStream(QObject *parent) : QObject(parent) {}

VideoStream::~VideoStream() {
  // Qt automatically disconnects signals/slots when a QObject is destroyed.
  // Manually disconnecting here risks dangling pointer crashes.
}

void VideoStream::setVideoSink(QVideoSink *sink) {
  if (m_videoSink != sink) {
    m_videoSink = sink;
    emit videoSinkChanged();
  }
}

void VideoStream::setSlotId(int id) {
  if (m_slotId != id) {
    m_slotId = id;
    emit slotIdChanged();
    tryAttach();
  }
}

void VideoStream::setRtspUrl(const QString &url) {
  if (m_rtspUrl != url) {
    m_rtspUrl = url;
    emit rtspUrlChanged();
    tryAttach();
  }
}

void VideoStream::tryAttach() {
  if (!videoManager)
    return; // Wait until initialized

  VideoWorker *newWorker = nullptr;
  if (m_slotId >= 0) {
    newWorker = videoManager->getWorkerBySlot(m_slotId);
    if (!newWorker && !m_rtspUrl.isEmpty()) {
      newWorker = videoManager->getWorker(m_rtspUrl);
    }
  } else if (!m_rtspUrl.isEmpty()) {
    newWorker = videoManager->getWorker(m_rtspUrl);
  }

  if (newWorker != m_worker) {
    if (!m_worker.isNull()) {
      disconnect(m_worker.data(), &VideoWorker::frameReady, this,
                 &VideoStream::handleNewFrame);
    }
    m_worker = newWorker;
    if (m_worker) {
      connect(m_worker, &VideoWorker::frameReady, this,
              &VideoStream::handleNewFrame, Qt::QueuedConnection);
    } else {
      // Listen for registration
      connect(videoManager, &VideoManager::workerRegistered, this,
              &VideoStream::onWorkerRegistered, Qt::UniqueConnection);
    }
  }
}

void VideoStream::onWorkerRegistered(const QString &url) {
  if (!videoManager)
    return;
  bool isTarget = false;
  if (m_slotId >= 0) {
    isTarget = (videoManager->slotUrl(m_slotId) == url);
  } else {
    isTarget = (m_rtspUrl == url);
  }

  if (isTarget) {
    if (videoManager) {
      disconnect(videoManager, &VideoManager::workerRegistered, this,
                 &VideoStream::onWorkerRegistered);
    }
    tryAttach();
  }
}

void VideoStream::handleNewFrame(const QVideoFrame &frame) {
  if (m_videoSink) {
    m_videoSink->setVideoFrame(frame);
  }
}
