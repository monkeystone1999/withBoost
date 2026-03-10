#pragma once

#include "VideoWorker.hpp"
#include <QPointer>
#include <QQuickItem>
#include <QRectF>
#include <QSGImageNode>
#include <QSGTexture>
#include <atomic>
#include <memory>
#include <vector>

// ── §2 VIDEO_STREAMING_SPEC: QSG Streaming Optimisation ──────────────────────
// Changes vs previous version:
//   - m_cropRect: normalised UV crop [0..1].  Default = full frame (0,0,1,1).
//   - m_cachedTex / m_cachedW / m_cachedH: render-thread texture reuse.
//     A new QSGTexture is only allocated when frame dimensions change (rare).
//     The same texture object is reused every frame → eliminates 312 GPU
//     allocations/sec that were caused by createTextureFromImage every frame.
//   - Q_PROPERTY cropRect: QML can bind tile UV coordinates.
// ─────────────────────────────────────────────────────────────────────────────
class VideoSurfaceItem : public QQuickItem {
  Q_OBJECT
  Q_PROPERTY(QObject *worker READ worker WRITE setWorker NOTIFY workerChanged)
  Q_PROPERTY(
      QRectF cropRect READ cropRect WRITE setCropRect NOTIFY cropRectChanged)

public:
  explicit VideoSurfaceItem(QQuickItem *parent = nullptr);
  ~VideoSurfaceItem() override;

  QObject *worker() const;
  void setWorker(QObject *obj);

  QRectF cropRect() const { return m_cropRect; }
  void setCropRect(const QRectF &r);

signals:
  void workerChanged();
  void cropRectChanged();

protected:
  QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private slots:
  void onFrameReady();

private:
  QPointer<VideoWorker> m_worker;

  // ── GUI-thread state ──────────────────────────────────────────────────────
  QRectF m_cropRect{0, 0, 1, 1}; // set by QML

  // ── Render-thread private — never touch from GUI thread ───────────────────
  QSGTexture *m_cachedTex{nullptr};

  int m_cachedW{0};
  int m_cachedH{0};
  std::shared_ptr<std::vector<uint8_t>> m_renderBufferHold;
  std::atomic<bool> m_updatePending{false};
};
