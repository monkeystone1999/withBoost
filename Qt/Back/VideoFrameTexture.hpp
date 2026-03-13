#pragma once

#include <QByteArray>
#include <QSGDynamicTexture>
#include <QSGTexture>
#include <QSize>
#include <QtGlobal>

class QRhi;
class QRhiTexture;
class QRhiResourceUpdateBatch;

// ==============================================================================
// VideoFrameTexture
//
// 핵심 역할: FFmpeg 디코딩된 CPU 버퍼를 GPU 텍스처로 효율적으로 업로드.
//
// 최적화 포인트:
// 1. QRhiTexture 재사용: 해상도 변경 시에만 텍스처 재생성 (D3D1 texture churn
// 방지)
// 2. NV12 지원: Y/UV 평면을 별도 텍스처로 업로드하여 GPU 메모리 및 버스 대역폭
// 50% 절감
// 3. Zero-copy: std::shared_ptr 교환을 통해 CPU 복사 없이 렌더 스레드에 프레임
// 전달
// ==============================================================================

class VideoFrameTexture : public QSGDynamicTexture {
public:
  enum class PixelFormat { RGBA };

  VideoFrameTexture();
  ~VideoFrameTexture() override;

  // 렌더 스레드에서 호출되어 최신 프레임을 예약함
  void setPendingFrame(std::shared_ptr<QByteArray> buf, int w, int h,
                       int strideY, int strideUV, int uvOffset,
                       PixelFormat format);

  // Scene Graph에 변경 사항이 있음을 알림
  bool updateTexture() override;

  // 실제 GPU 업로드가 수행되는 지점
  void
  commitTextureOperations(QRhi *rhi,
                          QRhiResourceUpdateBatch *resourceUpdates) override;

  // QSGTexture 인터페이스 구현
  qint64 comparisonKey() const override;
  QSize textureSize() const override { return size_; }
  bool hasAlphaChannel() const override { return false; }
  bool hasMipmaps() const override { return false; }
  QRhiTexture *rhiTexture() const override;

private:
  QRhiTexture *rhiTex_ = nullptr; // RGBA 모드용 단일 텍스처

  std::shared_ptr<QByteArray> pendingBuf_;
  std::shared_ptr<QByteArray> currentBuf_;

  int pendingW_ = 0;
  int pendingH_ = 0;
  int pendingStrideY_ = 0;
  int pendingStrideUV_ = 0;
  int pendingUVOffset_ = 0;
  PixelFormat pendingFormat_ = PixelFormat::RGBA;

  QSize size_;
  bool dirty_ = false;
  bool committedThisFrame_ = false;
};
