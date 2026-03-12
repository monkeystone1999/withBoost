#include "VideoSurfaceItem.hpp"
#include <algorithm>

VideoSurfaceItem::VideoSurfaceItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);

  updateTimer_ = new QTimer(this);
  updateTimer_->setInterval(33); // ~30 FPS
  connect(updateTimer_, &QTimer::timeout, this, [this]() { update(); });
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

  worker_ = w;

  if (worker_ && window()) {
    window_ = window();

    // ✅ Pull 기반 렌더링: beforeSynchronizing만 연결
    // Render Thread에서 렌더링 주기(~16.67ms for 60Hz)에 맞춰
    // 최신 프레임을 직접 pull. Signal/Slot 오버헤드 완전 제거.
    connect(window_, &QQuickWindow::beforeSynchronizing, this,
            &VideoSurfaceItem::onBeforeSynchronizing, Qt::DirectConnection);

    if (updateTimer_ && !updateTimer_->isActive()) {
      updateTimer_->start();
    }

    // ────────────────────────────────────────────────────────────────────
    // ❌ frameReady 연결 제거! (Pull 방식)
    //
    // 이전: connect(worker_, &VideoWorker::frameReady, this,
    //               [this]() { update(); }, Qt::AutoConnection);
    //
    // Pull 방식에서는 beforeSynchronizing이 렌더링 주기에 맞춰
    // 자동으로 최신 프레임을 읽어가므로 update() 호출 불필요.
    // Signal/Slot 오버헤드 제거 + UI stutter 완전 제거.
    // ────────────────────────────────────────────────────────────────────

  } else if (!worker_) {
    lastSeq_ = UINT64_MAX;
    if (updateTimer_)
      updateTimer_->stop();
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

  // 기존 window 연결 해제
  if (window_) {
    disconnect(window_, &QQuickWindow::beforeSynchronizing, this,
               &VideoSurfaceItem::onBeforeSynchronizing);
    window_ = nullptr;
  }

  // 새 window에 연결 (worker가 있을 때만)
  if (data.window && worker_) {
    window_ = data.window;

    // ✅ beforeSynchronizing만 연결 (Pull 방식)
    connect(window_, &QQuickWindow::beforeSynchronizing, this,
            &VideoSurfaceItem::onBeforeSynchronizing, Qt::DirectConnection);

    if (updateTimer_ && !updateTimer_->isActive()) {
      updateTimer_->start();
    }
  } else {
    if (updateTimer_)
      updateTimer_->stop();
  }
}

// ── onBeforeSynchronizing (Render Thread) ───────────────────────────────────
// GUI 스레드가 블로킹 중이므로 worker_ 접근 안전.
void VideoSurfaceItem::onBeforeSynchronizing() {
  if (!worker_ || !frameTex_)
    return;

  // seq 비교 — 바뀐 경우에만 프레임 교체
  const uint64_t seq = worker_->frameSeq();
  if (seq == lastSeq_)
    return; // 변경 없음

  lastSeq_ = seq;

  // 최신 프레임 읽기 (lock-free)
  VideoWorker::FrameData frame = worker_->getLatestFrame();

  if (!frame.buffer || frame.width <= 0 || frame.height <= 0)
    return;
  if (frame.stride <= 0)
    frame.stride = frame.width * 4;

  // VideoFrameTexture에 pending 프레임 전달 (zero-copy: 포인터 교환만)
  frameTex_->setPendingFrame(frame.buffer, frame.width, frame.height,
                              frame.stride);

  // QSGDynamicTexture dirty 마킹
  frameTex_->updateTexture();
  update();
}

// ── updatePaintNode (GUI Thread Sync) ───────────────────────────────────────
// QSGImageNode + RGBA 텍스처 (표준 Qt Scene Graph 방식)
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
    node->setFiltering(QSGTexture::Linear);
    node->setOwnsTexture(false); // 수명은 VSI가 관리

    if (!frameTex_) {
      frameTex_ = new VideoFrameTexture();
      // CRITICAL: First frame texture size initialization
      frameTex_->setPendingFrame(frame.buffer, frame.width, frame.height,
                                  frame.stride);
      frameTex_->updateTexture();
    }

    node->setTexture(frameTex_);
    nodeReady_ = true;
  }

  node->setRect(boundingRect());

  const qreal nx = std::clamp(cropRect_.x(), 0.0, 1.0);
  const qreal ny = std::clamp(cropRect_.y(), 0.0, 1.0);
  qreal nw = std::clamp(cropRect_.width(), 0.0, 1.0 - nx);
  qreal nh = std::clamp(cropRect_.height(), 0.0, 1.0 - ny);
  if (nw <= 0.0 || nh <= 0.0) {
    nw = 1.0;
    nh = 1.0;
  }
  node->setSourceRect(QRectF(nx * frame.width, ny * frame.height,
                             nw * frame.width, nh * frame.height));

  // ────────────────────────────────────────────────────────────────────────
  // CRITICAL: 재질(Texture) 업데이트 플래그
  //
  // 노드 지오메트리나 렌더 플래그가 변하지 않더라도,
  // frameTex_ 안에 새로운 프레임이 대기 중일 수 있으므로
  // 렌더러에게 텍스처(Material)가 변경되었음을 명시적으로 알려야 한다.
  // 이걸 호출해야 렌더러가 commitTextureOperations()를 실행한다.
  // ────────────────────────────────────────────────────────────────────────
  node->markDirty(QSGNode::DirtyMaterial);

  return node;
}
