#pragma once

#include <QSGDynamicTexture>
#include <QSize>
#include <memory>
#include <vector>

// -------------------------------------------------------------------
// VideoFrameTexture
//
// QSGDynamicTexture 서브클래스.
// 렌더 스레드 전용 — GUI 스레드에서 직접 접근 금지.
//
// 사용 흐름:
// [beforeSynchronizing 슬롯 — Render Thread]
// 1. setPendingFrame(buffer, w, h, stride) — 포인터 교환만, 무비용
// 2. updateTexture() — dirty = true, true 반환
// [commitTextureOperations — Render Thread]
// 3. QRhiTexture 재사용 + GPU 업로드 배치 등록
// -------------------------------------------------------------------
class VideoFrameTexture : public QSGDynamicTexture {
  Q_OBJECT
public:
  explicit VideoFrameTexture();
  ~VideoFrameTexture() override;

  // Render Thread에서 호출 — 포인터 교환만 (CPU 복사 없음)
  void setPendingFrame(std::shared_ptr<std::vector<uint8_t>> buf, int w, int h,
                       int stride);

  // QSGDynamicTexture 인터페이스
  bool updateTexture() override;

  // QSGTexture 인터페이스
  qint64 comparisonKey() const override;
  QRhiTexture *rhiTexture() const override;
  void
  commitTextureOperations(QRhi *rhi,
                          QRhiResourceUpdateBatch *resourceUpdates) override;

  QSize textureSize() const override { return size_; }
  bool hasAlphaChannel() const override { return false; }
  bool hasMipmaps() const override { return false; }

private:
  // 렌더 스레드 전용 상태
  QRhiTexture *rhiTex_ = nullptr; // GPU 텍스처 핸들 — 재사용
  QSize size_;                    // 현재 할당된 텍스처 크기
  bool dirty_ = false;            // commitTextureOperations 트리거

  // pending: setPendingFrame()에서 교환, commitTextureOperations에서 소비
  std::shared_ptr<std::vector<uint8_t>> pendingBuf_;

  // current: GPU가 업로드를 완전히 마칠 때까지 버퍼 수명을 유지하기 위함
  // (Zero-copy lifetime 연장)
  std::shared_ptr<std::vector<uint8_t>> currentBuf_;

  int pendingW_ = 0;
  int pendingH_ = 0;
  int pendingStride_ = 0;
};
