#include "VideoFrameTexture.hpp"
#include <rhi/qrhi.h>

VideoFrameTexture::VideoFrameTexture() {
  setFiltering(QSGTexture::Linear);
  setHorizontalWrapMode(QSGTexture::ClampToEdge);
  setVerticalWrapMode(QSGTexture::ClampToEdge);
}

VideoFrameTexture::~VideoFrameTexture() {
  // rhiTex(등)는 렌더 스레드 소유.
  // Qt scene graph 정리 완료 후 소멸자 호출이 보장됨.
  delete rhiTex_;
}

// ── setPendingFrame ──────────────────────────────────────────────────────────
// 렌더 스레드에서 호출 (beforeSynchronizing 슬롯 안에서).
// shared_ptr 교환 — ref-count +1만 발생, CPU 픽셀 복사 없음.
void VideoFrameTexture::setPendingFrame(std::shared_ptr<QByteArray> buf, int w,
                                        int h, int strideY, int strideUV,
                                        int uvOffset, PixelFormat format) {
  pendingBuf_ = std::move(buf);
  pendingW_ = w;
  pendingH_ = h;
  pendingStrideY_ = strideY;
  pendingStrideUV_ = strideUV;
  pendingUVOffset_ = uvOffset;
  pendingFormat_ = format;
  size_ = QSize(w, h); // CRITICAL: Update size immediately
}

// ── updateTexture() ──────────────────────────────────────────────────────────
// 렌더 스레드 전용. Scene Graph에 "이 텍스처 변경됨" 통지.
bool VideoFrameTexture::updateTexture() {
  if (!pendingBuf_ || pendingW_ <= 0 || pendingH_ <= 0)
    return false;
  dirty_ = true;
  committedThisFrame_ = false;
  return true;
}

// ── commitTextureOperations ──────────────────────────────────────────────────
// 실제 GPU 업로드가 수행되는 지점. NV12인 경우 두 개의 평면을 각각 업로드함.
void VideoFrameTexture::commitTextureOperations(
    QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) {
  if (!dirty_ || !pendingBuf_)
    return;
  if (committedThisFrame_)
    return;
  committedThisFrame_ = true;

  const QSize fullSize(pendingW_, pendingH_);

  if (!rhiTex_ || rhiTex_->pixelSize() != fullSize) {
    delete rhiTex_;
    rhiTex_ =
        rhi->newTexture(QRhiTexture::RGBA8, fullSize, 1, QRhiTexture::Flags{});
    rhiTex_->create();

    qDebug() << "Get New Texture != fullSize";
    size_ = fullSize;
  }

  currentBuf_ = std::move(pendingBuf_);
  auto buf = currentBuf_;
  QByteArray data = QByteArray::fromRawData(
      buf->constData(), static_cast<qsizetype>(pendingStrideY_ * pendingH_));

  QRhiTextureSubresourceUploadDescription sub(data);
  QRhiTextureUploadEntry entry(0, 0, sub);
  resourceUpdates->uploadTexture(rhiTex_, QRhiTextureUploadDescription(entry));

  dirty_ = false;
}

qint64 VideoFrameTexture::comparisonKey() const {
  if (rhiTex_)
    return qint64(rhiTex_);
  return qint64(this);
}

QRhiTexture *VideoFrameTexture::rhiTexture() const { return rhiTex_; }
