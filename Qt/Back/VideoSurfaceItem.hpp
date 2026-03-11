#pragma once

#include "VideoFrameTexture.hpp"
#include "VideoStream.hpp"
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QSGImageNode>

// ── VIDEO_STREAMING_SPEC: QSGDynamicTexture + beforeSynchronizing ────────────
//
// 이전 QTimer 방식 제거. 새로운 방식:
// Render Thread 의 beforeSynchronizing 단계에서 VideoWorker 로부터
// 최신 프레임을 lock-free 로 읽어들이고, m_frameTex 의 pending 버퍼만 교체한다.
// 실제 GPU 업로드는 GUI 블로킹 구간이 끝난 뒤 commitTextureOperations 에서
// QRhi 직접 제어를 통해 수행되어 GUI Thread Sync 지연을 완전히 소거한다.
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
  void itemChange(ItemChange change, const ItemChangeData &data) override;

private slots:
  // Qt::DirectConnection — Render Thread에서 실행
  void onBeforeSynchronizing();

private:
  QPointer<VideoWorker> m_worker;
  QPointer<QQuickWindow> m_window;

  // ── GUI-thread state ──────────────────────────────────────────────────────
  QRectF m_cropRect{0, 0, 1, 1};

  // ── Render-thread private — never touch from GUI thread ───────────────────
  VideoFrameTexture *m_frameTex{nullptr};
  uint64_t m_lastSeq{UINT64_MAX};
  bool m_nodeReady{false};
};
