#include "VideoSurfaceItem.hpp"
#include <QQuickWindow>
#include <QSGImageNode>

VideoSurfaceItem::VideoSurfaceItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

VideoSurfaceItem::~VideoSurfaceItem() {
  if (m_worker)
    disconnect(m_worker, &VideoWorker::frameReady, this,
               &VideoSurfaceItem::onFrameReady);
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

  emit workerChanged();
}

// ── GUI 스레드 슬롯
// ─────────────────────────────────────────────────────────── 하는 일: update()
// 호출 하나뿐 데이터 복사 없음, Mutex 없음, 메모리 할당 없음 update() 는 렌더
// 스레드에 "다음 프레임에 updatePaintNode 를 실행하라" 고 예약하고 즉시
// 반환한다 — GUI 이벤트 루프를 점유하지 않음
void VideoSurfaceItem::onFrameReady() {
  if (m_worker)
    update();
}

// ── 렌더 스레드에서 실행
// ────────────────────────────────────────────────────── GUI 스레드와 공유하는
// 데이터 없음 VideoWorker::getLatestFrame() 은 atomic load — lock-free
QSGNode *VideoSurfaceItem::updatePaintNode(QSGNode *oldNode,
                                           UpdatePaintNodeData *) {
  if (!m_worker)
    return oldNode;

  // atomic load — 뮤텍스 없이 FFmpeg 스레드가 쓴 최신 버퍼를 읽음
  VideoWorker::FrameData frame = m_worker->getLatestFrame();

  if (!frame.buffer || frame.width <= 0 || frame.height <= 0) {
    delete oldNode;
    return nullptr;
  }

  QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
  if (!node) {
    node = window()->createImageNode();
    node->setFiltering(QSGTexture::Linear);
    node->setOwnsTexture(true);
  }

  // BGRA → Format_ARGB32 (Qt 내부 포맷 일치, 추가 변환 없음)
  // QImage 는 buffer 를 복사하지 않고 포인터만 참조
  // shared_ptr 이 살아있는 동안 메모리 안전
  QImage img(frame.buffer->data(), frame.width, frame.height, frame.width * 4,
             QImage::Format_ARGB32);

  // createTextureFromImage: 렌더 스레드에서 실행
  // TextureCanUseAtlas를 추가하면 Qt 내부 텍스처 풀을 재사용하여
  // 매 프레임 GPU 메모리를 새로 할당/해제하는 부하를 획기적으로 줄입니다.
  QSGTexture *tex = window()->createTextureFromImage(
      img,
      QQuickWindow::CreateTextureOptions(QQuickWindow::TextureIsOpaque |
                                         QQuickWindow::TextureCanUseAtlas));
  if (tex)
    node->setTexture(tex);

  node->setRect(boundingRect());
  return node;
}
