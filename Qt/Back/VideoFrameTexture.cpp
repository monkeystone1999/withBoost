#include "VideoFrameTexture.hpp"
#include <rhi/qrhi.h>

VideoFrameTexture::VideoFrameTexture() {
  setFiltering(QSGTexture::Linear);
  setHorizontalWrapMode(QSGTexture::ClampToEdge);
  setVerticalWrapMode(QSGTexture::ClampToEdge);
}

VideoFrameTexture::~VideoFrameTexture() {
  // m_rhiTex는 렌더 스레드 소유.
  // Qt scene graph 정리 완료 후 소멸자 호출이 보장됨.
  delete rhiTex_;
}

// ── setPendingFrame ────────────────────────────────────────────────────
// Render Thread에서 호출 (beforeSynchronizing 슬롯 안에서).
// shared_ptr 교환 — ref-count +1만 발생, CPU 픽셀 복사 없음.
void VideoFrameTexture::setPendingFrame(
    std::shared_ptr<std::vector<uint8_t>> buf, int w, int h, int stride) {
  pendingBuf_ = std::move(buf);
  pendingW_ = w;
  pendingH_ = h;
  pendingStride_ = stride;
  size_ = QSize(w, h); // CRITICAL: Update size immediately so QSGImageNode UVs
                        // don't become NaN!
}

// ── updateTexture() ────────────────────────────────────────────────────
// Render Thread 전용.
// beforeSynchronizing 슬롯에서 setPendingFrame 직후 호출.
// true 반환 → SG가 이 텍스처를 사용하는 QSGImageNode를 dirty 처리
// → commitTextureOperations가 호출되어 GPU 업로드 수행
bool VideoFrameTexture::updateTexture() {
  if (!pendingBuf_ || pendingW_ <= 0 || pendingH_ <= 0)
    return false;
  dirty_ = true;
  return true; // Scene Graph에 "이 텍스처 변경됨" 통지
}

// ── commitTextureOperations ────────────────────────────────────────────
// Render Thread 전용.
// GUI Thread Sync에 포함되지 않음 — GUI를 블로킹하지 않음.
//
// 핵심: QRhiTexture 재사용.
// 해상도가 동일한 한 ID3D11Texture2D를 매 프레임 재사용.
// 크기 변경 시에만 delete + create 발생.
void VideoFrameTexture::commitTextureOperations(
    QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) {
  if (!dirty_ || !pendingBuf_)
    return;

  const QSize newSize(pendingW_, pendingH_);

  // 크기가 다를 때만 GPU 텍스처 핸들 재생성
  if (!rhiTex_ || rhiTex_->pixelSize() != newSize) {
    delete rhiTex_;
    rhiTex_ = rhi->newTexture(QRhiTexture::RGBA8, newSize,
                               /*sampleCount=*/1, QRhiTexture::Flags{});
    rhiTex_->create(); // D3D11: ID3D11Texture2D 할당 — 크기 변경 시에만
    size_ = newSize;
  }

  // GPU 업로드 배치 등록 — zero-copy
  // shared_ptr을 m_currentBuf로 넘겨 GPU 업로드가 끝날 때까지 버퍼 수명 보장
  currentBuf_ = std::move(pendingBuf_);
  auto buf = currentBuf_;
  QRhiTextureSubresourceUploadDescription sub;
  sub.setData(QByteArray::fromRawData(
      reinterpret_cast<const char *>(buf->data()),
      static_cast<qsizetype>(pendingStride_ * pendingH_)));

  resourceUpdates->uploadTexture(
      rhiTex_,
      QRhiTextureUploadDescription({QRhiTextureUploadEntry(0, 0, sub)}));

  dirty_ = false;
}

qint64 VideoFrameTexture::comparisonKey() const {
  return rhiTex_ ? qint64(rhiTex_) : qint64(this);
}

QRhiTexture *VideoFrameTexture::rhiTexture() const { return rhiTex_; }
