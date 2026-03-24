#include "VideoStream.hpp"
#include <QDebug>
#include <QMetaObject>
#include <QTimerEvent>

VideoWorker::VideoWorker(const QString &cameraId, QObject *parent)
    : QObject(parent), cameraId_(cameraId), video_(std::make_unique<Video>()) {

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

      while (frameSpinLock_.test_and_set(std::memory_order_acquire)) {
      }
      latestFrame_ = frame;
      frameSpinLock_.clear(std::memory_order_release);
      frameSeq_.fetch_add(1, std::memory_order_release);

      emit frameReady(frame);
    }
  };
}

VideoWorker::~VideoWorker() { stopStream(); }

void VideoWorker::startStream() {
  if (connectionString_.isEmpty())
    return;
  video_->startStream(connectionString_.toStdString(), fpsLimit_);
}
void VideoWorker::stopStream() { video_->stopStream(); }

VideoManager::VideoManager(QObject *parent) : QObject(parent) {}

VideoManager::~VideoManager() { clearAll(); }

VideoWorker *VideoManager::getWorker(const QString &cameraId) {
  return workers_.value(cameraId, nullptr);
}

VideoWorker *VideoManager::getWorkerBySlot(int slotId) {
  return getWorker(slotToCameraId_.value(slotId));
}

bool VideoManager::hasSlot(int slotId) const {
  return slotToCameraId_.contains(slotId);
}
QString VideoManager::slotCameraId(int slotId) const {
  return slotToCameraId_.value(slotId);
}

void VideoManager::clearAll() {
  for (auto *worker : workers_) {
    worker->stopStream();
    delete worker;
  }
  workers_.clear();
  slotToCameraId_.clear();
}

void VideoManager::registerSlots(const QList<SlotInfo> &Slots) {
  slotToCameraId_.clear();
  for (const SlotInfo &si : Slots)
    slotToCameraId_.insert(si.slotId, si.cameraId);

  for (const SlotInfo &si : Slots) {
    if (si.cameraId.isEmpty() || workers_.contains(si.cameraId))
      continue;
    auto *worker = new VideoWorker(si.cameraId, this);
    if (urlProvider_) {
      worker->setConnectionString(urlProvider_(si.cameraId));
    }
    if (fpsProvider_) {
      worker->setFpsLimit(fpsProvider_());
    }
    workers_.insert(si.cameraId, worker);
    worker->startStream();
    emit workerRegistered(si.cameraId);
  }
}

void VideoManager::registerCameraIds(const QStringList &cameraIds) {
  for (const QString &id : cameraIds) {
    if (id.isEmpty() || workers_.contains(id))
      continue;
    auto *worker = new VideoWorker(id, this);
    if (urlProvider_) {
      worker->setConnectionString(urlProvider_(id));
    }
    if (fpsProvider_) {
      worker->setFpsLimit(fpsProvider_());
    }
    workers_.insert(id, worker);
    worker->startStream();
    emit workerRegistered(id);
  }
}

void VideoManager::restartWorker(const QString &cameraId) {
  auto *worker = workers_.value(cameraId);
  if (!worker) {
    if (cameraId.isEmpty())
      return;
    worker = new VideoWorker(cameraId, this);
    if (urlProvider_) {
      worker->setConnectionString(urlProvider_(cameraId));
    }
    if (fpsProvider_) {
      worker->setFpsLimit(fpsProvider_());
    }
    workers_.insert(cameraId, worker);
    worker->startStream();
    emit workerRegistered(cameraId);
  } else {
    worker->stopStream();
    if (urlProvider_) {
      worker->setConnectionString(urlProvider_(cameraId));
    }
    if (fpsProvider_) {
      worker->setFpsLimit(fpsProvider_());
    }
    worker->startStream();
    emit workerRegistered(cameraId);
  }
}

void VideoManager::setFpsLimit(int fps) {
  for (auto *worker : workers_) {
    worker->setFpsLimit(fps);
  }
}

// ────────────────────────────────────────────────────────────────────────
// VideoStream (QML <-> VideoWorker Bridge)
// ────────────────────────────────────────────────────────────────────────

extern VideoManager *videoManager; // Required to fetch the worker

VideoStream::VideoStream(QObject *parent) : QObject(parent) {
  m_timerId = startTimer(16);
}

VideoStream::~VideoStream() {
  if (m_timerId) {
    killTimer(m_timerId);
  }
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

void VideoStream::setCameraId(const QString &id) {
  if (m_cameraId != id) {
    m_cameraId = id;
    emit cameraIdChanged();
    tryAttach();
  }
}

void VideoStream::tryAttach() {
  if (!videoManager)
    return; // Wait until initialized

  VideoWorker *newWorker = nullptr;
  if (m_slotId >= 0) {
    newWorker = videoManager->getWorkerBySlot(m_slotId);
    if (!newWorker && !m_cameraId.isEmpty()) {
      newWorker = videoManager->getWorker(m_cameraId);
    }
  } else if (!m_cameraId.isEmpty()) {
    newWorker = videoManager->getWorker(m_cameraId);
  }

  if (newWorker != m_worker) {
    m_worker = newWorker;
    m_lastSeq = 0;
    if (!m_worker) {
      // Listen for registration
      connect(videoManager, &VideoManager::workerRegistered, this,
              &VideoStream::onWorkerRegistered, Qt::UniqueConnection);
    }
  }
}

void VideoStream::onWorkerRegistered(const QString &id) {
  if (!videoManager)
    return;
  bool isTarget = false;
  if (m_slotId >= 0) {
    isTarget = (videoManager->slotCameraId(m_slotId) == id);
  } else {
    isTarget = (m_cameraId == id);
  }

  if (isTarget) {
    if (videoManager) {
      disconnect(videoManager, &VideoManager::workerRegistered, this,
                 &VideoStream::onWorkerRegistered);
    }
    tryAttach();
  }
}

void VideoStream::timerEvent(QTimerEvent *event) {
  if (event->timerId() == m_timerId) {
    if (m_worker && m_videoSink && !m_videoSink.isNull()) {
      uint64_t currentSeq = m_worker->frameSeq();
      if (currentSeq > m_lastSeq) {
        m_lastSeq = currentSeq;
        QVideoFrame frame = m_worker->getLatestFrame();
        if (frame.isValid()) {
          m_videoSink->setVideoFrame(frame);
        }
      }
    }
  } else {
    QObject::timerEvent(event);
  }
}
