#include "VideoSurfaceItem.hpp"
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>
#include <algorithm>
#include <cmath>
#include <qrhi.h>

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

// ── Render thread — updatePaintNode ──────────────────────────────────────────
// §2 optimisation: allocate a new QSGTexture ONLY when frame dimensions change.
// For a fixed-resolution RTSP stream, this happens exactly once per session.
// All subsequent frames reuse the same QSGTexture object → zero GPU alloc/free.
//
// §4 tile support: node->setSourceRect(m_cropRect) makes the GPU render only
// the specified sub-region — zero CPU pixel-copy cost.
// ── Custom QSGTexture for persistent RHI upload ──────────────────────────────
class VideoTexture : public QSGTexture {
public:
  VideoTexture(QQuickWindow *window) : m_window(window) {}
  ~VideoTexture() override {
    if (m_rhiTex)
      m_rhiTex->deleteLater();
  }

  void update(const QImage &image) {
    if (image.size() != m_size) {
      if (m_rhiTex)
        m_rhiTex->deleteLater();
      m_size = image.size();
      QRhi *rhi = m_window->rhi();
      if (rhi) {
        m_rhiTex = rhi->newTexture(QRhiTexture::RGBA8, m_size, 1,
                                   QRhiTexture::UsedWithSubresourceUpdates);
        m_rhiTex->create();
      }
    }
    m_pendingImage = image;
    m_dirty = true;
  }

  bool updateRhiTexture(QRhi *,
                        QRhiResourceUpdateBatch *resourceUpdates) override {
    if (m_dirty && m_rhiTex) {
      resourceUpdates->uploadTexture(m_rhiTex, m_pendingImage);
      m_dirty = false;
      return true;
    }
    return false;
  }

  QRhiTexture *rhiTexture() const override { return m_rhiTex; }
  QSize textureSize() const override { return m_size; }
  bool hasAlphaChannel() const override { return false; }
  bool hasMipmaps() const override { return false; }

private:
  QQuickWindow *m_window;
  QRhiTexture *m_rhiTex = nullptr;
  QSize m_size;
  QImage m_pendingImage;
  bool m_dirty = false;
};

QSGNode *VideoSurfaceItem::updatePaintNode(QSGNode *oldNode,
                                           UpdatePaintNodeData *) {
  m_updatePending.store(false, std::memory_order_release);

  if (!m_worker) {
    if (oldNode)
      delete oldNode;
    // Invalidate cached texture on worker detach
    m_cachedTex = nullptr;
    m_cachedW = 0;
    m_cachedH = 0;
    m_renderBufferHold.reset();
    return nullptr;
  }

  // atomic load — lock-free read from FFmpeg thread
  VideoWorker::FrameData frame = m_worker->getLatestFrame();

  if (!frame.buffer || frame.width <= 0 || frame.height <= 0) {
    m_renderBufferHold.reset();
    return oldNode;
  }
  if (frame.stride <= 0) {
    frame.stride = frame.width * 4;
  }

  QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
  if (!node) {
    node = window()->createImageNode();
    node->setFiltering(QSGTexture::Linear);
    node->setOwnsTexture(true); // node deletes m_cachedTex
    m_cachedTex = nullptr;
    m_cachedW = 0;
    m_cachedH = 0;
  }

  // §2: Persistent Texture Upload via custom VideoTexture
  VideoTexture *vTex = qobject_cast<VideoTexture *>(m_cachedTex);
  if (!vTex || frame.width != m_cachedW || frame.height != m_cachedH) {
    vTex = new VideoTexture(window());
    m_cachedTex = vTex;
    node->setTexture(m_cachedTex); // Takes ownership (setOwnsTexture is true)
    m_cachedW = frame.width;
    m_cachedH = frame.height;
  }

  m_renderBufferHold = frame.buffer;
  QImage img(m_renderBufferHold->data(), frame.width, frame.height,
             frame.stride, QImage::Format_RGBA8888);
  vTex->update(img);
  // In Qt 6, the renderer will automatically call vTex->updateRhiTexture()
  // before drawing because the texture is now used by the node's material.

  node->setRect(boundingRect());
  // §4: GPU crop — zero-CPU-cost tile sub-region rendering
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
