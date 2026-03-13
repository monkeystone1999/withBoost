#include "VideoSurfaceItem.hpp"
#include <QDebug>
#include <QSGImageNode>
#include <algorithm>
#include <chrono>
#include <rhi/qrhi.h>

VideoSurfaceItem::VideoSurfaceItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

VideoSurfaceItem::~VideoSurfaceItem() {
  if (frameTex_) {
    delete frameTex_;
    frameTex_ = nullptr;
  }
}

QObject *VideoSurfaceItem::worker() const { return worker_.data(); }

void VideoSurfaceItem::setWorker(QObject *obj) {
  VideoWorker *w = qobject_cast<VideoWorker *>(obj);
  if (worker_ == w)
    return;

  if (window_) {
    disconnect(window_, &QQuickWindow::beforeSynchronizing, this,
               &VideoSurfaceItem::onBeforeSynchronizing);
    window_ = nullptr;
  }

  // 타이머 정리
  if (updateTimer_) {
    updateTimer_->stop();
    delete updateTimer_;
    updateTimer_ = nullptr;
  }

  worker_ = w;

  if (worker_ && window()) {
    window_ = window();

    connect(window_, &QQuickWindow::beforeSynchronizing, this,
            &VideoSurfaceItem::onBeforeSynchronizing, Qt::DirectConnection);

    // Render loop이 잠들지 않도록 33ms(~30fps)마다 update() 호출
    updateTimer_ = new QTimer(this);
    updateTimer_->setInterval(33);
    connect(updateTimer_, &QTimer::timeout, this, [this]() { update(); });
    updateTimer_->start();

    update();

  } else if (!worker_) {
    lastSeq_ = UINT64_MAX;
    update();
  }

  emit workerChanged();
}

void VideoSurfaceItem::setCropRect(const QRectF &r) {
  if (cropRect_ == r)
    return;
  cropRect_ = r;
  emit cropRectChanged();
  update();
}

void VideoSurfaceItem::itemChange(ItemChange change,
                                  const ItemChangeData &data) {
  QQuickItem::itemChange(change, data);

  if (change != ItemSceneChange)
    return;

  if (window_) {
    disconnect(window_, &QQuickWindow::beforeSynchronizing, this,
               &VideoSurfaceItem::onBeforeSynchronizing);
    window_ = nullptr;
  }

  if (data.window && worker_) {
    window_ = data.window;

    connect(window_, &QQuickWindow::beforeSynchronizing, this,
            &VideoSurfaceItem::onBeforeSynchronizing, Qt::DirectConnection);
  }
}

void VideoSurfaceItem::onBeforeSynchronizing() {
  if (!worker_ || !frameTex_) {
    return;
  }

  const uint64_t seq = worker_->frameSeq();
  if (seq == lastSeq_) {
    // 프레임 업데이트 없음
    return;
  }

  // ── FPS 카운터 ──
  static std::chrono::steady_clock::time_point fpsT0 =
      std::chrono::steady_clock::now();
  static int fpsCount = 0;
  fpsCount++;
  auto now = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsT0)
          .count();
  if (elapsed >= 1000) {
    qWarning("[VideoSurfaceItem] QSG pull rate: %d frame/sec (seq=%llu)",
             fpsCount, (unsigned long long)seq);
    fpsCount = 0;
    fpsT0 = now;
  }

  lastSeq_ = seq;

  VideoWorker::FrameData frame = worker_->getLatestFrame();

  if (!frame.buffer || frame.width <= 0 || frame.height <= 0) {
    qWarning("[VideoSurfaceItem] buffer null after seq change!");
    return;
  }

  int strideY = frame.strideY > 0 ? frame.strideY : frame.width * 4;
  VideoFrameTexture::PixelFormat fmt = VideoFrameTexture::PixelFormat::RGBA;

  frameTex_->setPendingFrame(frame.buffer, frame.width, frame.height, strideY,
                             0, 0, fmt);

  frameTex_->updateTexture();

  update();
}

QSGNode *VideoSurfaceItem::updatePaintNode(QSGNode *oldNode,
                                           UpdatePaintNodeData *) {
  if (!worker_) {
    delete oldNode;
    nodeReady_ = false;
    return nullptr;
  }

  VideoWorker::FrameData frame = worker_->getLatestFrame();
  if (!frame.buffer || frame.width <= 0 || frame.height <= 0)
    return oldNode;

  QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
  if (!node) {
    node = window()->createImageNode();
    node->setOwnsTexture(false);
    if (!frameTex_) {
      frameTex_ = new VideoFrameTexture();
    }
    nodeReady_ = true;
  }

  if (!frameTex_)
    return node;

  int strideY = frame.strideY > 0 ? frame.strideY : frame.width * 4;
  VideoFrameTexture::PixelFormat fmt = VideoFrameTexture::PixelFormat::RGBA;

  frameTex_->setPendingFrame(frame.buffer, frame.width, frame.height, strideY,
                             0, 0, fmt);
  frameTex_->updateTexture();

  const qreal nx = std::clamp(cropRect_.x(), 0.0, 1.0);
  const qreal ny = std::clamp(cropRect_.y(), 0.0, 1.0);
  qreal nw = std::clamp(cropRect_.width(), 0.0, 1.0 - nx);
  qreal nh = std::clamp(cropRect_.height(), 0.0, 1.0 - ny);
  if (nw <= 0.0 || nh <= 0.0) {
    nw = 1.0;
    nh = 1.0;
  }

  // QSGImageNode handles source rect based on internal logic.
  // It expects unnormalized pixel coordinates when the backing texture API
  // supports it, or it transparently maps.
  QRectF srcRect(nx * frame.width, ny * frame.height, nw * frame.width,
                 nh * frame.height);

  node->setSourceRect(srcRect);
  node->setRect(boundingRect());
  node->setTexture(frameTex_);

  return node;
}
