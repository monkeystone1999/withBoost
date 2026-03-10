#include "VideoSurfaceItem.hpp"
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>
#include <algorithm>
#include <cmath>

VideoSurfaceItem::VideoSurfaceItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

VideoSurfaceItem::~VideoSurfaceItem() {
  if (m_worker)
    disconnect(m_worker, &VideoWorker::frameReady, this,
               &VideoSurfaceItem::onFrameReady);
  // m_cachedTex is render-thread owned; Qt scene graph will delete it via
  // the node destructor (node->setOwnsTexture(true)).
}

QObject *VideoSurfaceItem::worker() const { return m_worker.data(); }

void VideoSurfaceItem::setWorker(QObject *obj) {
  VideoWorker *w = qobject_cast<VideoWorker *>(obj);
  if (m_worker == w)
    return;

  if (m_worker)
    disconnect(m_worker, &VideoWorker::frameReady, this,
               &VideoSurfaceItem::onFrameReady);

  m_worker = w;

  if (m_worker)
    connect(m_worker, &VideoWorker::frameReady, this,
            &VideoSurfaceItem::onFrameReady, Qt::QueuedConnection);
  else
    update();

  emit workerChanged();
}

void VideoSurfaceItem::setCropRect(const QRectF &r) {
  if (m_cropRect == r)
    return;
  m_cropRect = r;
  emit cropRectChanged();
  update(); // schedule repaint so new crop takes effect immediately
}

// ── GUI thread slot — just schedules a repaint
// ────────────────────────────────
void VideoSurfaceItem::onFrameReady() {
  if (!m_worker || !isVisible() || !window() || !window()->isExposed())
    return;
  // Coalesce update requests so GUI thread does not enqueue redundant updates
  // when frameReady arrives faster than render consumption.
  if (m_updatePending.exchange(true, std::memory_order_acq_rel))
    return;
  update();
}

// ── 렌더 스레드: VSync 주기마다 Qt SG sync 단계에서 호출됨 ────────────
// QRhi 직접 접근 없음. createTextureFromImage → Qt 내부가 GPU 업로드 처리.
// §4 tile: setSourceRect 으로 GPU 서브리전 렌더링 (CPU 픽셀 복사 비용 없음).
QSGNode *VideoSurfaceItem::updatePaintNode(QSGNode *oldNode,
                                           UpdatePaintNodeData *) {
  m_updatePending.store(false, std::memory_order_release);

  // worker 없음: 노드 전체 해제, 상태 초기화
  if (!m_worker) {
    delete oldNode;        // node가 texture 소유 → 함께 해제
    m_cachedTex = nullptr; // 비소유 추적 포인터만 초기화
    m_cachedW = 0;
    m_cachedH = 0;
    m_renderBufferHold.reset();
    return nullptr;
  }

  // FFmpeg 스레드에서 lock-free로 최신 프레임 읽기
  VideoWorker::FrameData frame = m_worker->getLatestFrame();

  if (!frame.buffer || frame.width <= 0 || frame.height <= 0) {
    m_renderBufferHold.reset();
    return oldNode;
  }
  if (frame.stride <= 0)
    frame.stride = frame.width * 4;

  // QSGImageNode 재사용 — VSync마다 신규 생성 없음
  QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
  if (!node) {
    node = window()->createImageNode();
    node->setFiltering(QSGTexture::Linear);
    node->setOwnsTexture(true); // node가 texture 수명 관리
    m_cachedTex = nullptr;
    m_cachedW = 0;
    m_cachedH = 0;
  }

  // QImage 래핑 (zero-copy: shared_ptr로 버퍼 수명 연장)
  m_renderBufferHold = frame.buffer;
  const QImage img(m_renderBufferHold->data(), frame.width, frame.height,
                   frame.stride, QImage::Format_RGBA8888);

  // Qt 공식 경로: QRhi 없이 GPU 텍스처 업로드
  // TextureIsOpaque: FFmpeg RGBA 출력에 알파 없음 → 블렌딩 비용 절감
  QSGTexture *newTex =
      window()->createTextureFromImage(img, QQuickWindow::TextureIsOpaque);

  if (newTex) {
    node->setTexture(newTex); // 이전 texture는 node가 delete
    m_cachedTex = newTex;     // 비소유 추적 (절대 delete 금지)
    m_cachedW = frame.width;
    m_cachedH = frame.height;
  }

  node->setRect(boundingRect());
  // §4 tile cropRect: 정규화 좌표 → 픽셀 좌표 변환 후 GPU 서브리전 설정
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
  return node;
}
