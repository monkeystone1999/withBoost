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
  delete m_rhiTex;
}

// ── setPendingFrame ────────────────────────────────────────────────────
// Render Thread에서 호출 (beforeSynchronizing 슬롯 안에서).
// shared_ptr 교환 — ref-count +1만 발생, CPU 픽셀 복사 없음.
void VideoFrameTexture::setPendingFrame(
    std::shared_ptr<std::vector<uint8_t>> buf, int w, int h, int stride) {
  m_pendingBuf = std::move(buf);
  m_pendingW = w;
  m_pendingH = h;
  m_pendingStride = stride;
  m_size = QSize(w, h); // CRITICAL: Update size immediately so QSGImageNode UVs
                        // don't become NaN!
}

// ── updateTexture() ────────────────────────────────────────────────────
// Render Thread 전용.
// beforeSynchronizing 슬롯에서 setPendingFrame 직후 호출.
// true 반환 → SG가 이 텍스처를 사용하는 QSGImageNode를 dirty 처리
// → commitTextureOperations가 호출되어 GPU 업로드 수행
bool VideoFrameTexture::updateTexture() {
  if (!m_pendingBuf || m_pendingW <= 0 || m_pendingH <= 0)
    return false;
  m_dirty = true;
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
  if (!m_dirty || !m_pendingBuf)
    return;

  const QSize newSize(m_pendingW, m_pendingH);

  // 크기가 다를 때만 GPU 텍스처 핸들 재생성
  if (!m_rhiTex || m_rhiTex->pixelSize() != newSize) {
    delete m_rhiTex;
    m_rhiTex = rhi->newTexture(QRhiTexture::RGBA8, newSize,
                               /*sampleCount=*/1, QRhiTexture::Flags{});
    m_rhiTex->create(); // D3D11: ID3D11Texture2D 할당 — 크기 변경 시에만
    m_size = newSize;
  }

  // GPU 업로드 배치 등록 — zero-copy
  // shared_ptr을 m_currentBuf로 넘겨 GPU 업로드가 끝날 때까지 버퍼 수명 보장
  m_currentBuf = std::move(m_pendingBuf);
  auto buf = m_currentBuf;
  QRhiTextureSubresourceUploadDescription sub;
  sub.setData(QByteArray::fromRawData(
      reinterpret_cast<const char *>(buf->data()),
      static_cast<qsizetype>(m_pendingStride * m_pendingH)));

  resourceUpdates->uploadTexture(
      m_rhiTex,
      QRhiTextureUploadDescription({QRhiTextureUploadEntry(0, 0, sub)}));

  m_dirty = false;
}

qint64 VideoFrameTexture::comparisonKey() const {
  return m_rhiTex ? qint64(m_rhiTex) : qint64(this);
}

QRhiTexture *VideoFrameTexture::rhiTexture() const { return m_rhiTex; }
