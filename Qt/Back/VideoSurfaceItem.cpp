#include "VideoSurfaceItem.hpp"
#include <algorithm>

VideoSurfaceItem::VideoSurfaceItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

VideoSurfaceItem::~VideoSurfaceItem() {
  if (m_frameTex) {
    delete m_frameTex;
    m_frameTex = nullptr;
  }
}

QObject *VideoSurfaceItem::worker() const { return m_worker.data(); }

void VideoSurfaceItem::setWorker(QObject *obj) {
  VideoWorker *w = qobject_cast<VideoWorker *>(obj);
  if (m_worker == w)
    return;

  // 이전 연결 해제
  if (m_worker) {
    disconnect(m_worker.data(), &VideoWorker::frameReady, this, nullptr);
  }

  if (m_window) {
    disconnect(m_window, &QQuickWindow::beforeSynchronizing, this,
               &VideoSurfaceItem::onBeforeSynchronizing);
    m_window = nullptr;
  }

  m_worker = w;

  if (m_worker && window()) {
    m_window = window();

    // beforeSynchronizing: Render Thread에서 프레임 데이터 가져오기
    connect(m_window, &QQuickWindow::beforeSynchronizing, this,
            &VideoSurfaceItem::onBeforeSynchronizing, Qt::DirectConnection);

    // ────────────────────────────────────────────────────────────────────
    // CRITICAL: 실시간 렌더링 트리거
    //
    // VideoWorker의 frameReady 시그널을 받아서 update()를 호출.
    // 새 프레임이 도착할 때마다 scene graph가 다시 그려짐.
    //
    // Qt::AutoConnection: 스레드 경계를 넘으므로 Qt::QueuedConnection으로 자동
    // 전환. (Video 스레드에서 emit → GUI 스레드에서 update() 호출)
    //
    // zero-copy: shared_ptr atomic 교환만 사용, 메모리 복사 없음.
    // ────────────────────────────────────────────────────────────────────
    connect(
        m_worker.data(), &VideoWorker::frameReady, this, [this]() { update(); },
        Qt::AutoConnection);

  } else if (!m_worker) {
    m_lastSeq = UINT64_MAX;
    update();
  }

  emit workerChanged();
}

void VideoSurfaceItem::setCropRect(const QRectF &r) {
  if (m_cropRect == r)
    return;
  m_cropRect = r;
  emit cropRectChanged();
  update();
}

void VideoSurfaceItem::itemChange(ItemChange change,
                                  const ItemChangeData &data) {
  QQuickItem::itemChange(change, data);

  if (change != ItemSceneChange)
    return;

  // 기존 window 연결 해제
  if (m_window) {
    disconnect(m_window, &QQuickWindow::beforeSynchronizing, this,
               &VideoSurfaceItem::onBeforeSynchronizing);
    m_window = nullptr;
  }

  // 새 window에 연결 (worker가 있을 때만)
  if (data.window && m_worker) {
    m_window = data.window;

    // beforeSynchronizing: Render Thread에서 프레임 데이터 가져오기
    connect(m_window, &QQuickWindow::beforeSynchronizing, this,
            &VideoSurfaceItem::onBeforeSynchronizing, Qt::DirectConnection);

    // frameReady: GUI 스레드에서 update() 호출
    connect(
        m_worker.data(), &VideoWorker::frameReady, this, [this]() { update(); },
        Qt::AutoConnection);
  }
}

// ── onBeforeSynchronizing (Render Thread) ───────────────────────────────────
// GUI 스레드가 블로킹 중이므로 m_worker 접근 안전.
void VideoSurfaceItem::onBeforeSynchronizing() {
  if (!m_worker || !m_frameTex)
    return;

  // seq 비교 — 바뀐 경우에만 프레임 교체
  const uint64_t seq = m_worker->frameSeq();
  if (seq == m_lastSeq)
    return; // 변경 없음

  m_lastSeq = seq;

  // 최신 프레임 읽기 (lock-free)
  VideoWorker::FrameData frame = m_worker->getLatestFrame();

  if (!frame.buffer || frame.width <= 0 || frame.height <= 0)
    return;
  if (frame.stride <= 0)
    frame.stride = frame.width * 4;

  // VideoFrameTexture에 pending 프레임 전달 (zero-copy: 포인터 교환만)
  m_frameTex->setPendingFrame(frame.buffer, frame.width, frame.height,
                              frame.stride);

  // QSGDynamicTexture dirty 마킹
  m_frameTex->updateTexture();

  // update()는 frameReady 시그널에서 호출됨 (실시간 렌더링)
  // → 여기서 update() 호출 불필요
}

// ── updatePaintNode (GUI Thread Sync) ───────────────────────────────────────
// new/delete QSGTexture 제거, rect 및 crop 설정에만 집중.
QSGNode *VideoSurfaceItem::updatePaintNode(QSGNode *oldNode,
                                           UpdatePaintNodeData *) {
  if (!m_worker) {
    delete oldNode;
    m_nodeReady = false;
    return nullptr;
  }

  VideoWorker::FrameData frame = m_worker->getLatestFrame();
  if (!frame.buffer || frame.width <= 0 || frame.height <= 0)
    return oldNode;

  QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
  if (!node) {
    node = window()->createImageNode();
    node->setFiltering(QSGTexture::Linear);
    node->setOwnsTexture(false); // 수명은 VSI가 관리

    if (!m_frameTex) {
      m_frameTex = new VideoFrameTexture();
      // CRITICAL: First frame texture size initialization
      m_frameTex->setPendingFrame(frame.buffer, frame.width, frame.height,
                                  frame.stride);
      m_frameTex->updateTexture();
    }

    node->setTexture(m_frameTex);
    m_nodeReady = true;
  }

  node->setRect(boundingRect());

  const qreal nx = std::clamp(m_cropRect.x(), 0.0, 1.0);
  const qreal ny = std::clamp(m_cropRect.y(), 0.0, 1.0);
  qreal nw = std::clamp(m_cropRect.width(), 0.0, 1.0 - nx);
  qreal nh = std::clamp(m_cropRect.height(), 0.0, 1.0 - ny);
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
  // m_frameTex 안에 새로운 프레임이 대기 중일 수 있으므로
  // 렌더러에게 텍스처(Material)가 변경되었음을 명시적으로 알려야 한다.
  // 이걸 호출해야 렌더러가 commitTextureOperations()를 실행한다.
  // ────────────────────────────────────────────────────────────────────────
  node->markDirty(QSGNode::DirtyMaterial);

  return node;
}
