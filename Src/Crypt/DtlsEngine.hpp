#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <openssl/ssl.h>
#include <string>
#include <vector>

/**
 * @file DtlsEngine.hpp
 * @brief DTLS 및 MediaDTLS 공용 엔진
 *
 * 표준 DTLS 세션과 미디어 전용 부분 암호화(MediaDTLS) 세션을 관리합니다.
 */

namespace DTLS {

/// 쿠키 초기화 (앱 시작 시 1회 호출 필수)
void InitCookie();

/// 서버용 컨텍스트 생성
SSL_CTX *ServerContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/// 클라이언트용 컨텍스트 생성
SSL_CTX *ClientContext(const char *certfile = nullptr,
                       const char *keyfile = nullptr,
                       const char *cafile = nullptr);

/// DTLS 세션 관리 클래스
class Session {
public:
  Session(SSL_CTX *ctx, bool isServer = true,
          std::vector<uint8_t> peerIdent = {});
  ~Session();

  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;
  Session(Session &&other) noexcept;
  Session &operator=(Session &&other) noexcept;

  bool isValid() const { return ssl != nullptr; }
  bool isHandshakeDone() const;
  void setPeer(std::vector<uint8_t> peerIdent);

  std::vector<uint8_t> Handshake();
  std::vector<uint8_t> decrypt(const char *buffer, size_t size);
  std::vector<uint8_t> encrypt(const char *buffer, size_t size);
  std::vector<uint8_t> getHandshakeData();

private:
  void cleanup();
  std::vector<uint8_t> flushWriteBio();

  SSL *ssl = nullptr;
  BIO *readBio = nullptr;
  BIO *writeBio = nullptr;
  std::vector<uint8_t> peer;
  static constexpr int BufferSize = 4096;
};

} // namespace DTLS

namespace MediaDTLS {

// ─────────────────────────────────────────────
// 오버헤드 상수 (모두 명시적으로 분리)
// ─────────────────────────────────────────────
constexpr size_t OVERHEAD_IP = 20; // IPv4
constexpr size_t OVERHEAD_UDP = 8;
constexpr size_t OVERHEAD_DTLS_HEADER = 13; // DTLS 1.2 레코드 헤더
constexpr size_t OVERHEAD_GCM_TAG = 16;     // AES-GCM auth tag

// 실제 링크 MTU (1200 byte UDP 환경)
constexpr size_t LINK_MTU = 1200;

// ─────────────────────────────────────────────
// 패킷 헤더 구조체
// ─────────────────────────────────────────────
enum class MessageType : uint32_t {
  Image = 1,
  Audio = 2,
  Control = 3,
};

#pragma pack(push, 1)
struct PacketHeader {
  MessageType type; // 4 B
  uint32_t length;  // 4 B  — 뒤따르는 DTLS 암호문의 바이트 수
};

// uint8_t  → 최대 255 청크  ≈  288 KB  (MB 단위 이미지에서 오버플로우)
// uint16_t → 최대 65535 청크 ≈  72 MB  (충분)
struct ImageHeader : PacketHeader {
  uint16_t FrameNumber;       // 프레임 번호 (연속 스트리밍 대비 uint16_t)
  uint16_t SequenceNumber;    // 현재 청크 번호 (0-based)
  uint16_t MaxSequenceNumber; // 전체 청크 수 - 1
};
// sizeof(ImageHeader) = 4 + 4 + 2 + 2 + 2 = 14 B
#pragma pack(pop)

// ImageHeader 크기를 포함한 실제 최대 DTLS plaintext 크기
//   1200 - 20(IP) - 8(UDP) - 14(ImageHeader) - 13(DTLS hdr) - 16(GCM tag) =
//   1129 B
constexpr size_t MAX_DTLS_PAYLOAD = LINK_MTU - OVERHEAD_IP - OVERHEAD_UDP -
                                    sizeof(ImageHeader) - OVERHEAD_DTLS_HEADER -
                                    OVERHEAD_GCM_TAG;
// = 1129 B

// ─────────────────────────────────────────────
// 컨텍스트 생성
// ─────────────────────────────────────────────
inline SSL_CTX *ClientContext(const char *certfile = nullptr,
                              const char *keyfile = nullptr,
                              const char *cafile = nullptr) {
  SSL_CTX *ctx = SSL_CTX_new(DTLS_client_method());
  if (!ctx)
    return nullptr;

  SSL_CTX_set_ciphersuites(ctx, "TLS_AES_256_GCM_SHA384:"
                                "TLS_CHACHA20_POLY1305_SHA256:"
                                "TLS_AES_128_GCM_SHA256");

  SSL_CTX_set_cipher_list(
      ctx, "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
           "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
           "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256");

  if (certfile)
    SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM);
  if (keyfile)
    SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM);

  if (cafile) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
  }
  return ctx;
}

inline SSL_CTX *ServerContext(const char *certfile, const char *keyfile,
                              const char *cafile = nullptr) {
  SSL_CTX *ctx = SSL_CTX_new(DTLS_server_method());
  if (!ctx)
    return nullptr;

  SSL_CTX_set_ciphersuites(ctx, "TLS_AES_256_GCM_SHA384:"
                                "TLS_CHACHA20_POLY1305_SHA256:"
                                "TLS_AES_128_GCM_SHA256");

  SSL_CTX_set_cipher_list(
      ctx, "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
           "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
           "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256");

  if (certfile &&
      SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "MediaDTLS: 인증서 로드 실패\n";
  if (keyfile &&
      SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "MediaDTLS: 개인키 로드 실패\n";

  if (cafile) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       nullptr);
    SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
  }
  return ctx;
}

// ─────────────────────────────────────────────
// DTLS 세션
// ─────────────────────────────────────────────
class Session {
private:
  SSL *ssl = nullptr;
  BIO *readBio = nullptr;
  BIO *writeBio = nullptr;

  void cleanup() {
    if (ssl) {
      SSL_free(ssl); // SSL_free가 BIO도 함께 해제
      ssl = nullptr;
      readBio = writeBio = nullptr;
    }
  }

  // writeBio에 쌓인 암호화 패킷 1회 추출
  std::vector<uint8_t> flushWriteBio() {
    std::vector<uint8_t> out;
    int pending = BIO_pending(writeBio);
    if (pending > 0) {
      out.resize(pending);
      int n = BIO_read(writeBio, out.data(), pending);
      if (n > 0)
        out.resize(n);
      else
        out.clear();
    }
    return out;
  }

public:
  Session(SSL_CTX *ctx, bool isServer) {
    ssl = SSL_new(ctx);
    if (!ssl)
      return;

    if (isServer)
      SSL_set_accept_state(ssl);
    else
      SSL_set_connect_state(ssl);

    // 실제 링크 MTU를 그대로 전달 → OpenSSL이 DTLS 레코드 크기를 맞춤
    DTLS_set_link_mtu(ssl, static_cast<long>(LINK_MTU));

    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    BIO_set_mem_eof_return(readBio, -1);
    BIO_set_mem_eof_return(writeBio, -1);
    SSL_set_bio(ssl, readBio, writeBio);
  }

  ~Session() { cleanup(); }

  Session(Session &&o) noexcept
      : ssl(o.ssl), readBio(o.readBio), writeBio(o.writeBio) {
    o.ssl = nullptr;
    o.readBio = o.writeBio = nullptr;
  }

  Session &operator=(Session &&o) noexcept {
    if (this != &o) {
      cleanup();
      ssl = o.ssl;
      readBio = o.readBio;
      writeBio = o.writeBio;
      o.ssl = nullptr;
      o.readBio = o.writeBio = nullptr;
    }
    return *this;
  }

  bool isHandshakeDone() const { return ssl && SSL_is_init_finished(ssl); }

  std::string getNegotiatedCipher() const {
    if (!isHandshakeDone())
      return "Not Negotiated Yet";
    const SSL_CIPHER *c = SSL_get_current_cipher(ssl);
    return c ? SSL_CIPHER_get_name(c) : "Unknown";
  }

  std::vector<uint8_t> Handshake(const std::vector<uint8_t> &incoming = {}) {
    if (!ssl)
      return {};
    if (!incoming.empty())
      BIO_write(readBio, incoming.data(), static_cast<int>(incoming.size()));
    if (!SSL_is_init_finished(ssl))
      SSL_do_handshake(ssl);
    return flushWriteBio();
  }

  // ──────────────────────────────────────────
  // EncryptChunks
  //   반환: 각 원소가 UDP 1개에 실을 DTLS 암호문
  //   호출 측에서 원소마다 ImageHeader를 prepend해서 sendto()
  //
  //   5 MB 이미지 기준:
  //     5,242,880 / 1129 ≈ 4644 청크 → uint16_t(65535) 내 수용
  // ──────────────────────────────────────────
  std::vector<std::vector<uint8_t>> EncryptChunks(const uint8_t *data,
                                                  size_t size) {
    std::vector<std::vector<uint8_t>> result;
    if (!ssl || !SSL_is_init_finished(ssl) || !data || size == 0)
      return result;

    // 청크 수 사전 계산 → result 메모리 예약
    size_t chunkCount = (size + MAX_DTLS_PAYLOAD - 1) / MAX_DTLS_PAYLOAD;
    result.reserve(chunkCount);

    size_t offset = 0;
    while (offset < size) {
      size_t chunkSize = std::min(size - offset, MAX_DTLS_PAYLOAD);
      int written = SSL_write(ssl, data + offset, static_cast<int>(chunkSize));
      if (written <= 0) {
        std::cerr << "MediaDTLS: SSL_write 실패 (offset=" << offset << ")\n";
        break;
      }
      offset += static_cast<size_t>(written);

      // SSL_write 직후 즉시 flush → DTLS 레코드 1개씩 격리
      auto record = flushWriteBio();
      if (!record.empty())
        result.push_back(std::move(record));
    }
    return result;
  }

  // ──────────────────────────────────────────
  // Decrypt
  //   수신 측은 UDP 패킷에서 ImageHeader를 미리 제거한 뒤
  //   DTLS 암호문(packet)만 이 함수에 전달한다고 가정
  // ──────────────────────────────────────────
  std::vector<uint8_t> Decrypt(const uint8_t *packet, size_t size) {
    std::vector<uint8_t> plainText;
    if (!ssl || !packet || size == 0)
      return plainText;

    BIO_write(readBio, packet, static_cast<int>(size));

    if (!SSL_is_init_finished(ssl)) {
      SSL_do_handshake(ssl);
      if (!SSL_is_init_finished(ssl))
        return plainText;
    }

    uint8_t buf[4096];
    while (true) {
      int n = SSL_read(ssl, buf, sizeof(buf));
      if (n > 0) {
        plainText.insert(plainText.end(), buf, buf + n);
      } else {
        // SSL_ERROR_WANT_READ 등 — 더 읽을 데이터 없음
        break;
      }
    }
    return plainText;
  }
};

} // namespace MediaDTLS
