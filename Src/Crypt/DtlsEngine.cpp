#include "DtlsEngine.hpp"
#include <algorithm>
#include <iostream>
#include <openssl/hmac.h>
#include <openssl/rand.h>

namespace DTLS {

std::vector<uint8_t> SecretCookie(32);
std::once_flag cookie_flag;

void InitCookie() {
  std::call_once(cookie_flag, []() {
    if (RAND_bytes(SecretCookie.data(), SecretCookie.size()) <= 0) {
      std::cerr << "Fail to Init Cookie" << std::endl;
    }
  });
}

static int GenerateCookieWithPeer(const std::vector<uint8_t> &peer,
                                  unsigned char *cookie, unsigned int *len) {
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int resultlength;
  HMAC(EVP_sha256(), SecretCookie.data(), SecretCookie.size(), peer.data(),
       static_cast<int>(peer.size()), result, &resultlength);
  memcpy(cookie, result, resultlength);
  *len = resultlength;
  return 1;
}

static int VerifyCookieWithPeer(const std::vector<uint8_t> &peer,
                                const unsigned char *cookie, unsigned int len) {
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int resultlength;
  HMAC(EVP_sha256(), SecretCookie.data(), SecretCookie.size(), peer.data(),
       static_cast<int>(peer.size()), result, &resultlength);
  if (len == resultlength && memcmp(result, cookie, resultlength) == 0)
    return 1;
  return 0;
}

static int GenerateCookie(SSL *ssl, unsigned char *cookie, unsigned int *len) {
  std::vector<uint8_t> emptyPeer;
  return GenerateCookieWithPeer(emptyPeer, cookie, len);
}

static int VerifyCookie(SSL *ssl, const unsigned char *cookie,
                        unsigned int len) {
  std::vector<uint8_t> emptyPeer;
  return VerifyCookieWithPeer(emptyPeer, cookie, len);
}

SSL_CTX *ServerContext(const char *certfile, const char *keyfile,
                       const char *cafile) {
  SSL_CTX *ctx = SSL_CTX_new(DTLS_server_method());
  if (!ctx)
    return nullptr;
  if (certfile &&
      SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "인증서 로드 실패" << std::endl;
  if (keyfile &&
      SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "개인키 로드 실패" << std::endl;
  if (certfile && keyfile && SSL_CTX_check_private_key(ctx) != 1)
    std::cerr << "키 검증 실패" << std::endl;
  if (cafile) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       nullptr);
    SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
  }
  SSL_CTX_set_cookie_generate_cb(ctx, GenerateCookie);
  SSL_CTX_set_cookie_verify_cb(ctx, VerifyCookie);
  SSL_CTX_set_options(ctx, SSL_OP_COOKIE_EXCHANGE);
  return ctx;
}

SSL_CTX *ClientContext(const char *certfile, const char *keyfile,
                       const char *cafile) {
  SSL_CTX *ctx = SSL_CTX_new(DTLS_client_method());
  if (!ctx)
    return nullptr;
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

Session::Session(SSL_CTX *ctx, bool isServer, std::vector<uint8_t> peerIdent)
    : peer(std::move(peerIdent)) {
  ssl = SSL_new(ctx);
  if (!ssl)
    return;
  if (isServer)
    SSL_set_accept_state(ssl);
  else
    SSL_set_connect_state(ssl);
  readBio = BIO_new(BIO_s_mem());
  writeBio = BIO_new(BIO_s_mem());
  BIO_set_mem_eof_return(readBio, -1);
  BIO_set_mem_eof_return(writeBio, -1);
  SSL_set_bio(ssl, readBio, writeBio);
}

Session::~Session() { cleanup(); }

Session::Session(Session &&other) noexcept
    : ssl(other.ssl), readBio(other.readBio), writeBio(other.writeBio),
      peer(std::move(other.peer)) {
  other.ssl = nullptr;
  other.readBio = other.writeBio = nullptr;
}

Session &Session::operator=(Session &&other) noexcept {
  if (this != &other) {
    cleanup();
    ssl = other.ssl;
    readBio = other.readBio;
    writeBio = other.writeBio;
    peer = std::move(other.peer);
    other.ssl = nullptr;
    other.readBio = other.writeBio = nullptr;
  }
  return *this;
}

void Session::cleanup() {
  if (ssl) {
    SSL_free(ssl);
    ssl = nullptr;
    readBio = writeBio = nullptr;
  }
}

bool Session::isHandshakeDone() const {
  return ssl && SSL_is_init_finished(ssl);
}
void Session::setPeer(std::vector<uint8_t> peerIdent) {
  peer = std::move(peerIdent);
}

std::vector<uint8_t> Session::flushWriteBio() {
  std::vector<uint8_t> out;
  int pending = BIO_pending(writeBio);
  if (pending > 0) {
    out.resize(pending);
    int read = BIO_read(writeBio, out.data(), pending);
    if (read > 0)
      out.resize(read);
    else
      out.clear();
  }
  return out;
}

std::vector<uint8_t> Session::Handshake() {
  if (!ssl)
    return {};
  if (!SSL_is_init_finished(ssl)) {
    int res = SSL_do_handshake(ssl);
    if (res <= 0) {
      int err = SSL_get_error(ssl, res);
      if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
        std::cerr << "Handshake error: " << err << std::endl;
    }
  }
  return flushWriteBio();
}

std::vector<uint8_t> Session::decrypt(const char *buffer, size_t size) {
  std::vector<uint8_t> plainText;
  if (!ssl)
    return plainText;
  BIO_write(readBio, buffer, static_cast<int>(size));
  if (!SSL_is_init_finished(ssl)) {
    SSL_do_handshake(ssl);
    if (!SSL_is_init_finished(ssl))
      return plainText;
  }
  uint8_t buf[BufferSize];
  while (true) {
    int read = SSL_read(ssl, buf, sizeof(buf));
    if (read > 0)
      plainText.insert(plainText.end(), buf, buf + read);
    else
      break;
  }
  return plainText;
}

std::vector<uint8_t> Session::encrypt(const char *buffer, size_t size) {
  if (!ssl || !SSL_is_init_finished(ssl))
    return {};
  SSL_write(ssl, buffer, static_cast<int>(size));
  return flushWriteBio();
}

std::vector<uint8_t> Session::getHandshakeData() { return flushWriteBio(); }
} // namespace DTLS

namespace MediaDTLS {
// ─────────────────────────────────────────────
// 공통 cipher suite (TLS 1.3 / DTLS 1.3 우선, DTLS 1.2 fallback)
// ─────────────────────────────────────────────
static void applyCipherSuites(SSL_CTX *ctx) {
  // DTLS 1.3 / TLS 1.3
  SSL_CTX_set_ciphersuites(ctx, "TLS_AES_256_GCM_SHA384:"
                                "TLS_CHACHA20_POLY1305_SHA256:"
                                "TLS_AES_128_GCM_SHA256");

  // DTLS 1.2 fallback — ECDHE + GCM/Poly1305 만 허용
  SSL_CTX_set_cipher_list(
      ctx, "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
           "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
           "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256");
}

// ─────────────────────────────────────────────
// 컨텍스트 생성
// ─────────────────────────────────────────────
SSL_CTX *ServerContext(const char *certfile, const char *keyfile,
                       const char *cafile) {
  SSL_CTX *ctx = SSL_CTX_new(DTLS_server_method());
  if (!ctx)
    return nullptr;

  applyCipherSuites(ctx);

  if (certfile &&
      SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "DTLS: 인증서 로드 실패\n";
  if (keyfile &&
      SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
    std::cerr << "DTLS: 개인키 로드 실패\n";

  if (cafile) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       nullptr);
    SSL_CTX_load_verify_locations(ctx, cafile, nullptr);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
  }
  return ctx;
}

SSL_CTX *ClientContext(const char *certfile, const char *keyfile,
                       const char *cafile) {
  SSL_CTX *ctx = SSL_CTX_new(DTLS_client_method());
  if (!ctx)
    return nullptr;

  applyCipherSuites(ctx);

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

// ─────────────────────────────────────────────
// Session
// ─────────────────────────────────────────────
Session::Session(SSL_CTX *ctx, bool isServer) {
  ssl = SSL_new(ctx);
  if (!ssl)
    return;

  readBio = BIO_new(BIO_s_mem());
  writeBio = BIO_new(BIO_s_mem());
  BIO_set_mem_eof_return(readBio, -1);
  BIO_set_mem_eof_return(writeBio, -1);
  SSL_set_bio(ssl, readBio, writeBio);

  // UDP 환경 MTU 힌트 — IP 단편화 방지
  DTLS_set_link_mtu(ssl, 1200);

  if (isServer)
    SSL_set_accept_state(ssl);
  else
    SSL_set_connect_state(ssl);
}

Session::~Session() { cleanup(); }

Session::Session(Session &&other) noexcept
    : ssl(other.ssl), readBio(other.readBio), writeBio(other.writeBio) {
  other.ssl = nullptr;
  other.readBio = other.writeBio = nullptr;
}

Session &Session::operator=(Session &&other) noexcept {
  if (this != &other) {
    cleanup();
    ssl = other.ssl;
    readBio = other.readBio;
    writeBio = other.writeBio;
    other.ssl = nullptr;
    other.readBio = other.writeBio = nullptr;
  }
  return *this;
}

void Session::cleanup() {
  if (ssl) {
    SSL_free(ssl); // SSL_free가 BIO도 함께 해제
    ssl = nullptr;
    readBio = writeBio = nullptr;
  }
}

bool Session::isHandshakeDone() const {
  return ssl && SSL_is_init_finished(ssl);
}

std::string Session::getNegotiatedCipher() const {
  if (!isHandshakeDone())
    return "Not Negotiated Yet";
  const SSL_CIPHER *c = SSL_get_current_cipher(ssl);
  return c ? SSL_CIPHER_get_name(c) : "Unknown";
}

std::vector<uint8_t> Session::flushWriteBio() {
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

std::vector<uint8_t> Session::Handshake(const std::vector<uint8_t> &incoming) {
  if (!ssl)
    return {};
  if (!incoming.empty())
    BIO_write(readBio, incoming.data(), static_cast<int>(incoming.size()));
  if (!SSL_is_init_finished(ssl))
    SSL_do_handshake(ssl);
  return flushWriteBio();
}

std::vector<uint8_t> Session::encrypt(const char *buffer, int len) {
  if (!ssl || !SSL_is_init_finished(ssl) || !buffer || len <= 0)
    return {};
  SSL_write(ssl, buffer, len);
  return flushWriteBio();
}

std::vector<uint8_t> Session::decrypt(const char *buffer, int len) {
  std::vector<uint8_t> plainText;
  if (!ssl || !buffer || len <= 0)
    return plainText;

  BIO_write(readBio, buffer, len);

  if (!SSL_is_init_finished(ssl)) {
    SSL_do_handshake(ssl);
    if (!SSL_is_init_finished(ssl))
      return plainText; // 핸드셰이크 진행 중
  }

  uint8_t buf[BufferSize];
  while (true) {
    int n = SSL_read(ssl, buf, sizeof(buf));
    if (n > 0)
      plainText.insert(plainText.end(), buf, buf + n);
    else
      break;
  }
  return plainText;
}
} // namespace MediaDTLS

/**
 * @section Workflow Guide
 *
 * **[DtlsEngine 구현부 통합 가이드]**
 *
 * 1. 서버 측 통합 (MediaDTLS):
 *    - `MediaDTLS::ServerContext`를 호출하여 최적화된 SSL_CTX 생성.
 *    - 새 연결 발생 시 `MediaDTLS::Session` 생성.
 *    - `Handshake()` 반환값이 있으면 UDP로 전송.
 *
 * 2. 부수 효과 및 상태 관리:
 *    - `decrypt()` 호출 시 내부적으로 `readBio`에 데이터가 입력되며,
 *       핸드셰이크 중일 경우 `SSL_do_handshake`가 자동 진행됩니다.
 *    - 핸드셰이크 완료 전까지는 `encrypt()`가 빈 데이터를 반환하므로
 *       반드시 `isHandshakeDone()` 체크가 수반되어야 합니다.
 *
 * 3. 예시 루프:
 *    while(running) {
 *        recv_len = recvfrom(..., buf, ...);
 *        auto plain = session.decrypt(buf, recv_len);
 *        if (!plain.empty()) handle_payload(plain);
 *
 *        auto resp = session.flushWriteBio(); // 또는 Handshake() 결과
 *        if (!resp.empty()) sendto(..., resp.data(), ...);
 *    }
 */
