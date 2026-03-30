#include "VideoStream.hpp"
#include "../../Src/Domain/CameraManager.hpp"
#include "../../Src/Network/VideoEngine.hpp"
#include <QDebug>
#include <QMetaObject>
#include <QTimerEvent>


VideoWorker::VideoWorker(const QString &cameraId, QObject *parent)
    : QObject(parent), cameraId_(cameraId) {}

VideoWorker::~VideoWorker() { stopStream(); }

void VideoWorker::startStream() {
  if (!videoEngine_ || connectionString_.isEmpty())
    return;

  videoEngine_->onFrameReady =
      [this](const VideoEngine::FramePayload &payload) {
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

  videoEngine_->startStream(connectionString_.toStdString(), fpsLimit_);
}

void VideoWorker::stopStream() {
  if (videoEngine_)
    videoEngine_->stopStream();
}

void VideoWorker::setFpsLimit(int fps) {
  fpsLimit_ = fps;
  if (videoEngine_)
    videoEngine_->setFpsLimit(fps);
}

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
    if (cameraManager_) {
      auto *cam = cameraManager_->Get(si.cameraId.toStdString());
      if (cam && cam->video_)
        worker->setVideoEngine(cam->video_.get());
    }
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
    if (cameraManager_) {
      auto *cam = cameraManager_->Get(id.toStdString());
      if (cam && cam->video_)
        worker->setVideoEngine(cam->video_.get());
    }
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
    if (cameraManager_) {
      auto *cam = cameraManager_->Get(cameraId.toStdString());
      if (cam && cam->video_)
        worker->setVideoEngine(cam->video_.get());
    }
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

/**
 * @section Workflow Guide
 *
 * **[VideoStream 데이터 처리 기법 상세]**
 *
 * 1. NV12 프레임 변환 (Fast Path):
 *    - 코어 엔진의 FramePayload 데이터는 메모리 레이아웃이 NV12 형태로 고정되어
 * 있습니다.
 *    - `QVideoFrameFormat::Format_NV12`를 사용하여 중간 변환 과정 없이 Y, UV
 * 평면을 직접 복사함으로써 CPU 오버헤드를 극대화로 줄였습니다.
 *
 * 2. 스레드 동기화 (Lockless design 지향):
 *    - `VideoWorker`는 `std::atomic_flag` 기반의 스핀락을 사용하여 코어 엔진의
 * 쓰기 스레드와 `VideoStream`의 읽기 타이머 간의 데이터 정합성을 보장합니다.
 *    - 무거운 뮤텍스 대신 초고속 스핀락을 사용하여 비디오 레이턴시를
 * 최소화하였습니다.
 *
 * 3. 렌더링 스케줄링:
 *    - 실시간 60FPS 환경에서도 화면 깜빡임을 방지하기 위해 `m_lastSeq` 비교
 * 방식을 사용하여 이중 렌더링을 방지하고 정확히 새로운 프레임이 도착했을 때만
 * `QVideoSink`를 업데이트합니다.
 */
