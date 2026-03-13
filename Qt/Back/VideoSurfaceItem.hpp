#pragma once

#include "VideoFrameTexture.hpp"
#include "VideoStream.hpp"
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QSGImageNode>
#include <QSGTexture>
#include <QTimer>

// ── VIDEO_STREAMING_SPEC: Pull 기반 렌더링 ──────────────────────────────────
//
// 변경점 (Pull 기반 최적화):
// - frameReady signal 연결 제거 → beforeSynchronizing에서만 프레임 pull
// - Signal/Slot 오버헤드 제거로 UI stutter 완전 제거
// - 렌더링 주기(~16.67ms for 60Hz)에 정확히 동기화
//
// 유지된 것 (안정성):
// - sws_scale CPU RGBA 변환 (검증된 안정적 파이프라인)
// - QSGImageNode + RGBA 텍스처 (표준 Qt Scene Graph 방식)
// - VideoFrameTexture QRhi 텍스처 업로드
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

  QRectF cropRect() const { return cropRect_; }
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
  QPointer<VideoWorker> worker_;
  QPointer<QQuickWindow> window_;

  // ── GUI-thread state ──────────────────────────────────────────────────
  QRectF cropRect_{0, 0, 1, 1};

  // ── Render-thread private — never touch from GUI thread ───────────────
  VideoFrameTexture *frameTex_{nullptr};
  uint64_t lastSeq_{UINT64_MAX};
  bool nodeReady_{false};

  // Timer to drive the render loop
  QTimer *updateTimer_{nullptr};
};
