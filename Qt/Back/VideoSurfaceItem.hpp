#pragma once

#include "VideoWorker.hpp"
#include <QPointer>
#include <QQuickItem>
#include <QSGImageNode>
#include <QSGTexture>

// ── 설계 원칙 ────────────────────────────────────────────────────────────────
// GUI 스레드(Main Thread) 가 하는 일:
//   1. frameReady 시그널 수신
//   2. update() 호출 (렌더 예약, 즉시 반환)
//   끝. 데이터 복사/이동 없음. Mutex 없음.
//
// 렌더 스레드가 하는 일:
//   1. updatePaintNode() 에서 worker->getLatestFrame() atomic load
//   2. createTextureFromImage() → GPU 업로드
//   GUI 스레드와 데이터 공유 없음 (atomic 으로만 VideoWorker 와 통신)
// ─────────────────────────────────────────────────────────────────────────────
class VideoSurfaceItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *worker READ worker WRITE setWorker NOTIFY workerChanged)

public:
    explicit VideoSurfaceItem(QQuickItem *parent = nullptr);
    ~VideoSurfaceItem() override;

    QObject *worker() const;
    void setWorker(QObject *obj);

signals:
    void workerChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private slots:
    // GUI 스레드에서 실행 — update() 만 호출하고 즉시 반환
    void onFrameReady();

private:
    QPointer<VideoWorker> m_worker;
    // 렌더 스레드 전용 — GUI 스레드에서 접근 안 함
    // QMutex, m_pendingFrame, m_frameDirty 모두 제거
};
